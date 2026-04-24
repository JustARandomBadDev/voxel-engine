#include "graphics/buffer.h"

#include <stdexcept>

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include "graphics/device.h" 
#include "graphics/renderer.h"
#include "graphics/swapchain.h" 
#include "graphics/buffer_manager.h"

Buffer::Buffer(Buffer&& other) noexcept
: buffer(other.buffer), bufferMemory(other.bufferMemory), size(other.size), _device(other._device)
{
    other.buffer = VK_NULL_HANDLE;
    other.bufferMemory = VK_NULL_HANDLE;
    other.size = 0;
    other._device = nullptr;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this == &other) return *this;

    cleanup();

    buffer = other.buffer;
    bufferMemory = other.bufferMemory;
    size = other.size;
    _device = other._device;

    other.buffer = VK_NULL_HANDLE;
    other.bufferMemory = VK_NULL_HANDLE;
    other.size = 0;
    other._device = nullptr;

    return *this;
}

void Buffer::createBuffer(VkDeviceSize psize, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, Device& p_device) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = psize;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(p_device.getDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("[ERROR] Buffer::createBuffer() -> failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(p_device.getDevice(), buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = BufferManager::findMemoryType(memRequirements.memoryTypeBits, properties, p_device);

    if (vkAllocateMemory(p_device.getDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("[ERROR] Buffer::createBuffer() -> failed to allocate buffer memory!");
    }

    vkBindBufferMemory(p_device.getDevice(), buffer, bufferMemory, 0);

    size = psize;
    _device = &p_device;
}

void Buffer::cleanup() {
    if (_device != nullptr) {
        if (buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(_device->getDevice(), buffer, nullptr);
        }

        if (bufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(_device->getDevice(), bufferMemory, nullptr);
        }
    }

    buffer = VK_NULL_HANDLE;
    bufferMemory = VK_NULL_HANDLE;
    size = 0;
    _device = nullptr;
}
