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

#pragma once

#include "command_record.h"
#include "command_replay.h"
#include "common/helpers.h"
#include "common/vk_common.h"
#include "core/buffer.h"
#include "core/image.h"
#include "core/image_view.h"
#include "core/sampler.h"
#include "rendering/pipeline_state.h"
#include "rendering/render_target.h"

namespace vkb
{
class CommandPool;

/**
 * @brief Records Vulkan commands after begin function and replays them before end function is called.
 *        Helper class to build graphics/compute pipelines and descriptor sets
 */
class CommandBuffer : public NonCopyable
{
  public:
	enum class ResetMode
	{
		ResetPool,
		ResetIndividually,
		AlwaysAllocate,
	};

	enum class State
	{
		Invalid,
		Initial,
		Recording,
		Executable,
	};

	CommandBuffer(CommandPool &command_pool, VkCommandBufferLevel level);

	~CommandBuffer();

	/**
	 * @brief Move constructs
	 */
	CommandBuffer(CommandBuffer &&other);

	Device &get_device();

	CommandRecord &get_recorder();

	CommandReplay &get_replayer();

	const VkCommandBuffer &get_handle() const;

	bool is_recording() const;

	/**
	 * @brief Sets the command buffer so that it is ready for recording
	 *        If it is a secondary command buffer, a pointer to the
	 *        primary command buffer it inherits from must be provided
	 * @brief primary_cmd_buf (optional)
	 */
	VkResult begin(VkCommandBufferUsageFlags flags, CommandBuffer *primary_cmd_buf = nullptr);

	VkResult end();

	void begin_render_pass(const RenderTarget &render_target, const std::vector<LoadStoreInfo> &load_store_infos, const std::vector<VkClearValue> &clear_values, VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);

	void next_subpass();

	void resolve_subpasses();

	void execute_commands(std::vector<CommandBuffer *> &secondary_command_buffers);

	void end_render_pass();

	void bind_pipeline_layout(PipelineLayout &pipeline_layout);

	template <class T>
	void set_specialization_constant(uint32_t constant_id, const T &data);

	void set_specialization_constant(uint32_t constant_id, const std::vector<uint8_t> &data);

	void push_constants(uint32_t offset, const std::vector<uint8_t> &values);

	template <typename T>
	void push_constants(uint32_t offset, const T &value)
	{
		push_constants(offset,
		               std::vector<uint8_t>{reinterpret_cast<const uint8_t *>(&value),
		                                    reinterpret_cast<const uint8_t *>(&value) + sizeof(T)});
	}

	void bind_buffer(const core::Buffer &buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t array_element);

	void bind_image(const core::ImageView &image_view, const core::Sampler &sampler, uint32_t set, uint32_t binding, uint32_t array_element);

	void bind_input(const core::ImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element);

	void bind_vertex_buffers(uint32_t first_binding, const std::vector<std::reference_wrapper<const vkb::core::Buffer>> &buffers, const std::vector<VkDeviceSize> &offsets);

	void bind_index_buffer(const core::Buffer &buffer, VkDeviceSize offset, VkIndexType index_type);

	void set_viewport_state(const ViewportState &state_info);

	void set_vertex_input_state(const VertexInputState &state_info);

	void set_input_assembly_state(const InputAssemblyState &state_info);

	void set_rasterization_state(const RasterizationState &state_info);

	void set_multisample_state(const MultisampleState &state_info);

	void set_depth_stencil_state(const DepthStencilState &state_info);

	void set_color_blend_state(const ColorBlendState &state_info);

	void set_viewport(uint32_t first_viewport, const std::vector<VkViewport> &viewports);

	void set_scissor(uint32_t first_scissor, const std::vector<VkRect2D> &scissors);

	void set_line_width(float line_width);

	void set_depth_bias(float depth_bias_constant_factor, float depth_bias_clamp, float depth_bias_slope_factor);

	void set_blend_constants(const std::array<float, 4> &blend_constants);

	void set_depth_bounds(float min_depth_bounds, float max_depth_bounds);

	void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);

	void draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance);

	void draw_indexed_indirect(const core::Buffer &buffer, VkDeviceSize offset, uint32_t draw_count, uint32_t stride);

	void dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z);

	void dispatch_indirect(const core::Buffer &buffer, VkDeviceSize offset);

	void update_buffer(const core::Buffer &buffer, VkDeviceSize offset, const std::vector<uint8_t> &data);

	void blit_image(const core::Image &src_img, const core::Image &dst_img, const std::vector<VkImageBlit> &regions);

	void copy_image(const core::Image &src_img, const core::Image &dst_img, const std::vector<VkImageCopy> &regions);

	void copy_buffer_to_image(const core::Buffer &buffer, const core::Image &image, const std::vector<VkBufferImageCopy> &regions);

	void image_memory_barrier(const core::ImageView &image_view, const ImageMemoryBarrier &memory_barrier);

	void buffer_memory_barrier(const core::Buffer &buffer, VkDeviceSize offset, VkDeviceSize size, const BufferMemoryBarrier &memory_barrier);

	const State get_state() const;

	const VkCommandBufferUsageFlags get_usage_flags() const;

	/**
	 * @brief Reset the command buffer to a state where it can be recorded to
	 * @param reset_mode How to reset the buffer, should match the one used by the pool to allocate it
	 */
	VkResult reset(ResetMode reset_mode);

	const VkCommandBufferLevel level;

  private:
	State state{State::Initial};

	CommandPool &command_pool;

	VkCommandBuffer handle{VK_NULL_HANDLE};

	CommandRecord recorder;

	CommandReplay replayer;

	VkCommandBufferUsageFlags usage_flags{};
};

template <class T>
inline void CommandBuffer::set_specialization_constant(uint32_t constant_id, const T &data)
{
	set_specialization_constant(constant_id,
	                            {reinterpret_cast<const uint8_t *>(&data),
	                             reinterpret_cast<const uint8_t *>(&data) + sizeof(T)});
}

template <>
inline void CommandBuffer::set_specialization_constant<bool>(std::uint32_t constant_id, const bool &data)
{
	std::uint32_t value = static_cast<std::uint32_t>(data);

	set_specialization_constant(
	    constant_id,
	    {reinterpret_cast<const uint8_t *>(&value),
	     reinterpret_cast<const uint8_t *>(&value) + sizeof(std::uint32_t)});
}
}        // namespace vkb
