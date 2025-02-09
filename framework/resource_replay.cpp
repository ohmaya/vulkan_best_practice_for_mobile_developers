/* Copyright (c) 2019, Arm Limited and Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "resource_replay.h"

#include "common/logging.h"
#include "common/vk_common.h"
#include "resource_cache.h"
#include "resource_record.h"

namespace vkb
{
namespace
{
inline void read_subpass_info(std::istringstream &is, std::vector<SubpassInfo> &value)
{
	std::size_t size;
	read(is, size);
	value.resize(size);
	for (SubpassInfo &subpass : value)
	{
		read(is, subpass.input_attachments);
		read(is, subpass.output_attachments);
	}
}

inline void read_processes(std::istringstream &is, std::vector<std::string> &value)
{
	std::size_t size;
	read(is, size);
	value.resize(size);
	for (std::string &item : value)
	{
		read(is, item);
	}
}
}        // namespace

ResourceReplay::ResourceReplay()
{
	stream_resources[ResourceType::ShaderModule]     = std::bind(&ResourceReplay::create_shader_module, this, std::placeholders::_1, std::placeholders::_2);
	stream_resources[ResourceType::PipelineLayout]   = std::bind(&ResourceReplay::create_pipeline_layout, this, std::placeholders::_1, std::placeholders::_2);
	stream_resources[ResourceType::RenderPass]       = std::bind(&ResourceReplay::create_render_pass, this, std::placeholders::_1, std::placeholders::_2);
	stream_resources[ResourceType::GraphicsPipeline] = std::bind(&ResourceReplay::create_graphics_pipeline, this, std::placeholders::_1, std::placeholders::_2);
}

void ResourceReplay::play(ResourceCache &resource_cache, ResourceRecord &recorder)
{
	std::istringstream stream{recorder.get_stream().str()};

	while (true)
	{
		// Read command id
		ResourceType resource_type;
		read(stream, resource_type);

		if (stream.eof())
		{
			break;
		}

		// Find command function for the given command id
		auto cmd_it = stream_resources.find(resource_type);

		// Check if command replayer supports the given command
		if (cmd_it != stream_resources.end())
		{
			// Run command function
			cmd_it->second(resource_cache, stream);
		}
		else
		{
			LOGE("Replay command not supported.");
		}
	}
}

void ResourceReplay::create_shader_module(ResourceCache &resource_cache, std::istringstream &stream)
{
	VkShaderStageFlagBits    stage{};
	std::vector<uint8_t>     glsl_code;
	std::string              entry_point;
	std::string              preamble;
	std::vector<std::string> processes;

	read(stream,
	     stage,
	     glsl_code,
	     entry_point,
	     preamble);

	read_processes(stream, processes);

	ShaderSource  shader_source(std::move(glsl_code));
	ShaderVariant shader_variant(std::move(preamble), std::move(processes));

	auto &shader_module = resource_cache.request_shader_module(stage, shader_source, shader_variant);

	shader_modules.push_back(&shader_module);
}

void ResourceReplay::create_pipeline_layout(ResourceCache &resource_cache, std::istringstream &stream)
{
	std::vector<size_t> shader_indices;

	read(stream,
	     shader_indices);

	std::vector<ShaderModule *> shader_stages(shader_indices.size());
	std::transform(shader_indices.begin(), shader_indices.end(), shader_stages.begin(),
	               [&](size_t shader_index) { return shader_modules.at(shader_index); });
	auto &pipeline_layout = resource_cache.request_pipeline_layout(shader_stages);

	pipeline_layouts.push_back(&pipeline_layout);
}

void ResourceReplay::create_render_pass(ResourceCache &resource_cache, std::istringstream &stream)
{
	std::vector<Attachment>    attachments;
	std::vector<LoadStoreInfo> load_store_infos;
	std::vector<SubpassInfo>   subpasses;

	read(stream,
	     attachments,
	     load_store_infos);

	read_subpass_info(stream, subpasses);

	auto &render_pass = resource_cache.request_render_pass(attachments, load_store_infos, subpasses);

	render_passes.push_back(&render_pass);
}

void ResourceReplay::create_graphics_pipeline(ResourceCache &resource_cache, std::istringstream &stream)
{
	size_t   pipeline_layout_index{};
	size_t   render_pass_index{};
	uint32_t subpass_index{};

	read(stream,
	     pipeline_layout_index,
	     render_pass_index,
	     subpass_index);

	std::map<uint32_t, std::vector<uint8_t>> specialization_constant_state{};
	read(stream,
	     specialization_constant_state);

	VertexInputState vertex_input_sate{};

	read(stream,
	     vertex_input_sate.attributes,
	     vertex_input_sate.bindings);

	InputAssemblyState input_assembly_state{};
	RasterizationState rasterization_state{};
	ViewportState      viewport_state{};
	MultisampleState   multisample_state{};
	DepthStencilState  depth_stencil_state{};

	read(stream,
	     input_assembly_state,
	     rasterization_state,
	     viewport_state,
	     multisample_state,
	     depth_stencil_state);

	ColorBlendState color_blend_state{};

	read(stream,
	     color_blend_state.logic_op,
	     color_blend_state.logic_op_enable,
	     color_blend_state.attachments);

	PipelineState pipeline_state{};
	pipeline_state.set_pipeline_layout(*pipeline_layouts.at(pipeline_layout_index));
	pipeline_state.set_render_pass(*render_passes.at(render_pass_index));

	for (auto &item : specialization_constant_state)
	{
		pipeline_state.set_specialization_constant(item.first, item.second);
	}

	pipeline_state.set_subpass_index(subpass_index);
	pipeline_state.set_vertex_input_state(vertex_input_sate);
	pipeline_state.set_input_assembly_state(input_assembly_state);
	pipeline_state.set_rasterization_state(rasterization_state);
	pipeline_state.set_viewport_state(viewport_state);
	pipeline_state.set_multisample_state(multisample_state);
	pipeline_state.set_depth_stencil_state(depth_stencil_state);
	pipeline_state.set_color_blend_state(color_blend_state);

	auto &graphics_pipeline = resource_cache.request_graphics_pipeline(pipeline_state);

	graphics_pipelines.push_back(&graphics_pipeline);
}
}        // namespace vkb
