#ifndef VULKAN_BUFFER_H
#define VULKAN_BUFFER_H

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

struct Vertex;
class Device;

class Buffer {
public:
    Buffer() = default;
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, Device& p_device);
    
    void cleanup();

    VkBuffer& getBuffer() { return buffer; }
    const VkBuffer& getBuffer() const { return buffer; }
    VkDeviceMemory& getBufferMemory() { return bufferMemory; }
    const VkDeviceMemory& getBufferMemory() const { return bufferMemory; }
    VkDeviceSize getSize() const { return size; }

private:
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
    VkDeviceSize size = 0;
    Device* _device = nullptr;
};

#endif // BUFFER_MANAGER_HPP
