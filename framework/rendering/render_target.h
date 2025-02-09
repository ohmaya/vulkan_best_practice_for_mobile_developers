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

#include "common/helpers.h"
#include "common/vk_common.h"
#include "core/image.h"
#include "core/image_view.h"

namespace vkb
{
class Device;

/**
 * @brief Description of render pass attachments.
 * Attachment descriptions can be used to automatically create render target images.
 */
struct Attachment
{
	VkFormat format{VK_FORMAT_UNDEFINED};

	VkSampleCountFlagBits samples{VK_SAMPLE_COUNT_1_BIT};

	VkImageUsageFlags usage{VK_IMAGE_USAGE_SAMPLED_BIT};

	Attachment() = default;

	Attachment(VkFormat format, VkSampleCountFlagBits samples, VkImageUsageFlags usage);
};

/**
 * @brief RenderTarget contains three vectors for: core::Image, core::ImageView and Attachment.
 * The first two are Vulkan images and corresponding image views respectively.
 * Attachment (s) contain a description of the images, which has two main purposes:
 * - RenderPass creation only needs a list of Attachment (s), not the actual images, so we keep
 *   the minimum amount of information necessary
 * - Creation of a RenderTarget becomes simpler, because the caller can just ask for some
 *   Attachment (s) without having to create the images
 */
class RenderTarget : public NonCopyable
{
  public:
	using CreateFunc = std::function<RenderTarget(core::Image &&)>;

	static const CreateFunc DEFAULT_CREATE_FUNC;

	RenderTarget(std::vector<core::Image> &&images);

	RenderTarget(RenderTarget &&) = default;

	RenderTarget &operator=(RenderTarget &&other) noexcept;

	const VkExtent2D &get_extent() const;

	const std::vector<core::ImageView> &get_views() const;

	const std::vector<Attachment> &get_attachments() const;

	/**
	 * @brief Sets the current input attachments overwriting the current ones
	 *        Should be set before beginning the render pass and before starting a new subpass
	 * @param input Set of attachment reference number to use as input
	 */
	void set_input_attachments(std::vector<uint32_t> &input);

	const std::vector<uint32_t> &get_input_attachments() const;

	/**
	 * @brief Sets the current output attachments overwriting the current ones
	 *        Should be set before beginning the render pass and before starting a new subpass
	 * @param output Set of attachment reference number to use as output
	 */
	void set_output_attachments(std::vector<uint32_t> &output);

	const std::vector<uint32_t> &get_output_attachments() const;

  private:
	Device &device;

	VkExtent2D extent{};

	std::vector<core::Image> images;

	std::vector<core::ImageView> views;

	std::vector<Attachment> attachments;

	/// By default there are no input attachments
	std::vector<uint32_t> input_attachments = {};

	/// By default the output attachments is attachment 0
	std::vector<uint32_t> output_attachments = {0};
};
}        // namespace vkb
