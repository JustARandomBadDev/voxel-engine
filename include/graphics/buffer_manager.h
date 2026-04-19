#ifndef VULKAN_BUFFER_MANAGER_H
#define VULKAN_BUFFER_MANAGER_H

#include <vulkan/vulkan.h>
#include <vector>

#include "engine/mesh.h"
#include "engine/voxel_engine_config.h"
#include "graphics/buffer.h"
#include "graphics/uniform_buffer.h"
#include "graphics/block_update.h"
#include "graphics/allocators_manager.h"

struct CopyInfo {
    VkBuffer srcBuffer;
    VkBuffer dstBuffer;
    VkDeviceSize size;
    VkDeviceSize srcOffset;
    VkDeviceSize dstOffset;
};

class Renderer;
class Device;

class BufferManager {
public:
    void configure(Device& p_device, Renderer& p_renderer);

    void createBuffers(const GpuAllocatorConfig& p_gpu_allocator_config);
    void cleanupBuffers();

    void createUniformBuffers(uint32_t p_frames_in_flight);
    void updateUniformBuffer(uint32_t p_current_frame, glm::vec3 camPos, glm::mat4 matrix, glm::vec3 sunPos, glm::vec3 moonPos);
    void cleanupUniformBuffer();

    void applyCopies();

    static void copyBuffer(Buffer& srcBuffer, Buffer& dstBuffer, VkDeviceSize size);
    static void copyBuffer(Buffer& srcBuffer, Buffer& dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset);
    static VkCommandBuffer beginSingleTimeCommands(Renderer& p_renderer, Device& p_device);
    static void endSingleTimeCommands(VkCommandBuffer commandBuffer, Renderer& p_renderer, Device& p_device);

    static void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, Device& p_device);
    static uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, Device& p_device);


    AllocatorManager& getAllocator()         { return _opaque_allocator; }
    Buffer& getVertexBuffers()               { return _opaque_allocator.getVertexBuffer(); }
    Buffer& getIndexBuffers()                { return _opaque_allocator.getIndexBuffer(); }

    AllocatorManager& getTransparentAllocator() { return _transparent_allocator; }
    Buffer& getTransparentVertexBuffers()               { return _transparent_allocator.getVertexBuffer(); }
    Buffer& getTransparentIndexBuffers()                { return _transparent_allocator.getIndexBuffer(); }

    Buffer& getVoxelBuffer()                 { return voxelBuffer; }
    Buffer& getUpdateVoxelBuffer()           { return updateVoxelBuffer; }
    UniformBuffer& getUniformBuffer(int i)   { return uniformBuffers[i]; }




private:
    AllocatorManager _opaque_allocator;
    AllocatorManager _transparent_allocator;


    Buffer voxelBuffer;
    Buffer updateVoxelBuffer;
    std::vector<UniformBuffer> uniformBuffers;

    Device* _device = nullptr;
    Renderer* _renderer = nullptr;
    
    static std::vector<CopyInfo> pendingCopy;

};

#endif
