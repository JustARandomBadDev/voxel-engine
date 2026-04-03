#ifndef VULKAN_BUFFER_H
#define VULKAN_BUFFER_H

#include <vulkan/vulkan.h>
#include <vector>

#include <glm/glm.hpp>

struct Vertex;
class Device;

class Buffer {
public:
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, Device& p_device);
    
    void cleanup();

    VkBuffer& getBuffer() { return buffer; }
    VkDeviceMemory& getBufferMemory() { return bufferMemory; }
    VkDeviceSize getSize() { return size; }

private:
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
    VkDeviceSize size;
    Device* _device = nullptr;
};

#endif // BUFFER_MANAGER_HPP
