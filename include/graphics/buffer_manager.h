#ifndef VULKAN_BUFFER_MANAGER_H
#define VULKAN_BUFFER_MANAGER_H

#include <vulkan/vulkan.h>
#include <vector>

#include "engine/voxel_engine_config.h"
#include "graphics/buffer.h"
#include "graphics/uniform_buffer.h"
#include "graphics/allocators_manager.h"

class Renderer;
class Device;

// Owns persistent managed GPU buffers, per-frame uniform buffers, and the shared staging/upload path for allocator-backed mesh uploads.
class BufferManager {
public:
    // Pending uploads own copied CPU bytes until applyCopies() records and submits the staging-buffer transfer.
    struct PendingUpload {
        uint32_t dstBufferId = INVALID_BUFFER_ID;
        VkDeviceSize dstOffset = 0;
        std::vector<char> data;
    };

    void configure(Device& p_device, Renderer& p_renderer);

    void createBuffers(const GpuAllocatorConfig& p_gpu_allocator_config);
    void cleanupBuffers();

    void createUniformBuffers(uint32_t p_frames_in_flight);
    void updateUniformBuffer(uint32_t p_current_frame, glm::vec3 camPos, glm::mat4 matrix, glm::vec3 sunPos, glm::vec3 moonPos);
    void cleanupUniformBuffer();

    void applyCopies();

    uint32_t createManagedBuffer(VkDeviceSize p_size, VkBufferUsageFlags p_usage, VkMemoryPropertyFlags p_properties);
    Buffer& getManagedBuffer(uint32_t p_buffer_id);
    const Buffer& getManagedBuffer(uint32_t p_buffer_id) const;

    void enqueueUpload(uint32_t p_dst_buffer_id, VkDeviceSize p_dst_offset, const void* p_data, VkDeviceSize p_size);

    static VkCommandBuffer beginSingleTimeCommands(Renderer& p_renderer, Device& p_device);
    static void endSingleTimeCommands(VkCommandBuffer commandBuffer, Renderer& p_renderer, Device& p_device);

    static void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, Device& p_device);
    static uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, Device& p_device);

    AllocatorManager& getAllocator() { return _opaque_allocator; }
    const AllocatorManager& getAllocator() const { return _opaque_allocator; }

    AllocatorManager& getTransparentAllocator() { return _transparent_allocator; }
    const AllocatorManager& getTransparentAllocator() const { return _transparent_allocator; }

    UniformBuffer& getUniformBuffer(int i) { return uniformBuffers[i]; }
    const UniformBuffer& getUniformBuffer(int i) const { return uniformBuffers[i]; }

private:
    AllocatorManager _opaque_allocator;
    AllocatorManager _transparent_allocator;
    std::vector<UniformBuffer> uniformBuffers;
    std::vector<Buffer> _managed_buffers;
    Buffer _staging;

    Device* _device = nullptr;
    Renderer* _renderer = nullptr;
    bool _configured = false;
    std::vector<PendingUpload> _pending_uploads;

    void ensureConfigured(const char* p_caller) const;
    void ensureValidManagedBufferId(uint32_t p_buffer_id, const char* p_caller) const;
};

#endif
