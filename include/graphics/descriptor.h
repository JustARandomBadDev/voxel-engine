#ifndef DESCRIPTOR_MANAGER_H
#define DESCRIPTOR_MANAGER_H

#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>

class BufferManager;
class ComputePipeline;
class Device;
class GraphicPipeline;
class Texture;

class Descriptor {
public:
    void createDescriptorPool(Device& p_device, uint32_t p_frames_in_flight);
    void createDescriptorSets(BufferManager& p_buffer_manager, Texture& p_texture, GraphicPipeline& p_graphic_pipeline, ComputePipeline& p_compute_pipeline, Device& p_device, uint32_t p_frames_in_flight);
    void cleanup(Device& p_device);

    const std::vector<VkDescriptorSet>& getDescriptorSets() const { return descriptorSets; }
    const VkDescriptorSet& getComputeDescriptorSets(uint32_t p_current_frame) const { return computeDescriptorSets[p_current_frame]; }

private:
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkDescriptorSet> computeDescriptorSets;
};

#endif // DESCRIPTOR_MANAGER_H
