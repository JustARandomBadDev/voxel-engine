#ifndef VULKAN_BUFFER_H
#define VULKAN_BUFFER_H

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

struct Vertex;
class Device;

class Buffer {
public:
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, Device& p_device);
    
    void cleanup();

    VkBuffer& getBuffer() { return buffer; }
    const VkBuffer& getBuffer() const { return buffer; }
    VkDeviceMemory& getBufferMemory() { return bufferMemory; }
    const VkDeviceMemory& getBufferMemory() const { return bufferMemory; }
    VkDeviceSize getSize() const { return size; }

private:
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
    VkDeviceSize size;
    Device* _device = nullptr;
};

#endif // BUFFER_MANAGER_HPP
