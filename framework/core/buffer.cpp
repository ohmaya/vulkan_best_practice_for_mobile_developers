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

#include "buffer.h"

#include "device.h"

namespace vkb
{
namespace core
{
Buffer::Buffer(Device &device, VkDeviceSize size, VkBufferUsageFlags buffer_usage, VmaMemoryUsage memory_usage, VmaAllocationCreateFlags flags) :
    device{device},
    size{size}
{
	VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	buffer_info.usage = buffer_usage;
	buffer_info.size  = size;

	VmaAllocationCreateInfo memory_info{};
	memory_info.usage = memory_usage;
	memory_info.flags = flags;

	VmaAllocationInfo alloc_info{};
	auto              result = vmaCreateBuffer(device.get_memory_allocator(),
                                  &buffer_info, &memory_info,
                                  &handle, &memory,
                                  &alloc_info);

	if (flags & VMA_ALLOCATION_CREATE_MAPPED_BIT)
	{
		// No need to unmap in this case
		mapped_data = static_cast<uint8_t *>(alloc_info.pMappedData);
	}

	if (result != VK_SUCCESS)
	{
		throw VulkanException{result, "Cannot create Buffer"};
	}
}

Buffer::Buffer(Buffer &&other) :
    device{other.device},
    handle{other.handle},
    memory{other.memory},
    size{other.size},
    mapped_data{other.mapped_data},
    mapped{other.mapped}
{
	// Reset other handles to avoid releasing on destruction
	other.handle      = VK_NULL_HANDLE;
	other.memory      = VK_NULL_HANDLE;
	other.mapped_data = nullptr;
	other.mapped      = false;
}

Buffer::~Buffer()
{
	if (handle != VK_NULL_HANDLE && memory != VK_NULL_HANDLE)
	{
		unmap();
		vmaDestroyBuffer(device.get_memory_allocator(), handle, memory);
	}
}

const Device &Buffer::get_device() const
{
	return device;
}

VkBuffer Buffer::get_handle() const
{
	return handle;
}

VmaAllocation Buffer::get_memory() const
{
	return memory;
}

VkDeviceSize Buffer::get_size() const
{
	return size;
}

uint8_t *Buffer::map()
{
	if (!mapped_data)
	{
		VK_CHECK(vmaMapMemory(device.get_memory_allocator(), memory, reinterpret_cast<void **>(&mapped_data)));
		mapped = true;
	}
	return mapped_data;
}

void Buffer::flush() const
{
	vmaFlushAllocation(device.get_memory_allocator(), memory, 0, size);
}

void Buffer::unmap()
{
	if (mapped)
	{
		vmaUnmapMemory(device.get_memory_allocator(), memory);
		mapped_data = nullptr;
		mapped      = false;
	}
}

void Buffer::update(const std::vector<uint8_t> &data, size_t offset)
{
	update(data.data(), data.size(), offset);
}

void Buffer::update(const uint8_t *src, const size_t size, const size_t offset)
{
	map();
	std::copy(src, src + size, mapped_data + offset);
	flush();
#ifdef VK_USE_PLATFORM_MACOS_MVK
	unmap();        // Mac MoltenVK requires unmapping
#endif              // VK_USE_PLATFORM_MACOS_MVK
}

}        // namespace core
}        // namespace vkb
