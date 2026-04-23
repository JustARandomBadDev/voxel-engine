#ifndef DESCRIPTOR_MANAGER_H
#define DESCRIPTOR_MANAGER_H

#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>

class BufferManager;
class Device;
class GraphicPipeline;
class Texture;

// Owns the graphics descriptor pool and one descriptor set per swapchain image.
class Descriptor {
public:
    void createDescriptorPool(Device& p_device, uint32_t p_image_count);
    void createDescriptorSets(BufferManager& p_buffer_manager, Texture& p_texture, GraphicPipeline& p_graphic_pipeline, Device& p_device, uint32_t p_image_count);
    void cleanup(Device& p_device);

    const std::vector<VkDescriptorSet>& getDescriptorSets() const { return descriptorSets; }

private:
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;
};

#endif // DESCRIPTOR_MANAGER_H
