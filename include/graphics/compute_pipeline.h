#ifndef VULKAN_COMPUTE_H 
#define VULKAN_COMPUTE_H

#include <filesystem>
#include <vulkan/vulkan.h>

class Descriptor;
class Device;
class GraphicPipeline;
class Renderer;

class ComputePipeline {
public:
    void createComputePipeline(
        const std::filesystem::path& compute_shader_path,
        GraphicPipeline& p_graphic_pipeline,
        Device& p_device
    );
    void dispatchCompute(Descriptor& p_descriptor, Renderer& p_renderer, Device& p_device);
    void createDescriptorSetLayout(Device& p_device);
    void cleanup(Device& p_device);
    void cleanupDescriptorSetLayout(Device& p_device);

    const VkPipeline& getComputePipeline() const { return computePipeline; };
    const VkPipelineLayout& getComputePipelineLayout() const { return pipelineLayout; };
    const VkDescriptorSetLayout& getDescriptorSetLayout() const { return descriptorSetLayout; }

private:
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipeline computePipeline;
    VkPipelineLayout pipelineLayout;

    VkPipelineShaderStageCreateInfo createShaderStage(VkShaderModule shaderModule, VkShaderStageFlagBits stage);
};

#endif
