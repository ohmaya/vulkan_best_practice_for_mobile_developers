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

#include "resource_record.h"

#include "resource_cache.h"

namespace vkb
{
namespace
{
inline void write_subpass_info(std::ostringstream &os, const std::vector<SubpassInfo> &value)
{
	write(os, value.size());
	for (const SubpassInfo &item : value)
	{
		write(os, item.input_attachments);
		write(os, item.output_attachments);
	}
}

inline void write_processes(std::ostringstream &os, const std::vector<std::string> &value)
{
	write(os, value.size());
	for (const std::string &item : value)
	{
		write(os, item);
	}
}
}        // namespace

void ResourceRecord::set_data(const std::vector<uint8_t> &data)
{
	stream.str(std::string{data.begin(), data.end()});
}

std::vector<uint8_t> ResourceRecord::get_data()
{
	std::string str = stream.str();

	return std::vector<uint8_t>{str.begin(), str.end()};
}

const std::ostringstream &ResourceRecord::get_stream()
{
	return stream;
}

size_t ResourceRecord::register_shader_module(VkShaderStageFlagBits stage, const ShaderSource &glsl_source, const std::string &entry_point, const ShaderVariant &shader_variant)
{
	shader_module_indices.push_back(shader_module_indices.size());

	write(stream, ResourceType::ShaderModule, stage, glsl_source.get_data(), entry_point, shader_variant.get_preamble());

	write_processes(stream, shader_variant.get_processes());

	return shader_module_indices.back();
}

size_t ResourceRecord::register_pipeline_layout(const std::vector<ShaderModule *> &shader_modules)
{
	pipeline_layout_indices.push_back(pipeline_layout_indices.size());

	std::vector<size_t> shader_indices(shader_modules.size());
	std::transform(shader_modules.begin(), shader_modules.end(), shader_indices.begin(),
	               [this](ShaderModule *shader_module) { return shader_module_to_index.at(shader_module); });
	write(stream,
	      ResourceType::PipelineLayout,
	      shader_indices);

	return pipeline_layout_indices.back();
}

size_t ResourceRecord::register_render_pass(const std::vector<Attachment> &attachments, const std::vector<LoadStoreInfo> &load_store_infos, const std::vector<SubpassInfo> &subpasses)
{
	render_pass_indices.push_back(render_pass_indices.size());

	write(stream,
	      ResourceType::RenderPass,
	      attachments,
	      load_store_infos);

	write_subpass_info(stream, subpasses);

	return render_pass_indices.back();
}

size_t ResourceRecord::register_graphics_pipeline(VkPipelineCache /*pipeline_cache*/, PipelineState &pipeline_state)
{
	graphics_pipeline_indices.push_back(graphics_pipeline_indices.size());

	auto &pipeline_layout = pipeline_state.get_pipeline_layout();
	auto  render_pass     = pipeline_state.get_render_pass();

	write(stream,
	      ResourceType::GraphicsPipeline,
	      pipeline_layout_to_index.at(&pipeline_layout),
	      render_pass_to_index.at(render_pass),
	      pipeline_state.get_subpass_index());

	auto &specialization_constant_state = pipeline_state.get_specialization_constant_state().get_specialization_constant_state();

	write(stream,
	      specialization_constant_state);

	auto &vertex_input_state = pipeline_state.get_vertex_input_state();

	write(stream,
	      vertex_input_state.attributes,
	      vertex_input_state.bindings);

	write(stream,
	      pipeline_state.get_input_assembly_state(),
	      pipeline_state.get_rasterization_state(),
	      pipeline_state.get_viewport_state(),
	      pipeline_state.get_multisample_state(),
	      pipeline_state.get_depth_stencil_state());

	auto &color_blend_state = pipeline_state.get_color_blend_state();

	write(stream,
	      color_blend_state.logic_op,
	      color_blend_state.logic_op_enable,
	      color_blend_state.attachments);

	return graphics_pipeline_indices.back();
}

void ResourceRecord::set_shader_module(size_t index, const ShaderModule &shader_module)
{
	shader_module_to_index[&shader_module] = index;
}

void ResourceRecord::set_pipeline_layout(size_t index, const PipelineLayout &pipeline_layout)
{
	pipeline_layout_to_index[&pipeline_layout] = index;
}

void ResourceRecord::set_render_pass(size_t index, const RenderPass &render_pass)
{
	render_pass_to_index[&render_pass] = index;
}

void ResourceRecord::set_graphics_pipeline(size_t index, const GraphicsPipeline &graphics_pipeline)
{
	graphics_pipeline_to_index[&graphics_pipeline] = index;
}

}        // namespace vkb
