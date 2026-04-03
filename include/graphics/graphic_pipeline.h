#ifndef VULKAN_GRAPHIC_PIPELINE_H
#define VULKAN_GRAPHIC_PIPELINE_H

#include <vulkan/vulkan.h>
#include <vector>
#include <fstream>

#include "graphics/light.h"

class Device;
class Swapchain;

class GraphicPipeline {
public:
    void createRenderPass(Swapchain& p_swapchain, Device& p_device);
    void createDescriptorSetLayout(Device& p_device);
    void createGraphicsPipeline(Device& p_device);
    void cleanup(Device& p_device);
    void cleanupDescriptorSetLayout(Device& p_device);

    VkShaderModule createShaderModule(const std::vector<char>& code, Device& p_device);
    static std::vector<char> readFile(const std::string& filename);

    VkRenderPass getRenderPass() const { return renderPass; }

    const VkDescriptorSetLayout& getDescriptorSetLayout() { return descriptorSetLayout; }

    const VkPipeline& getOpaquePipeline() { return opaquePipeline; }
    const VkPipelineLayout& getOpaquePipelineLayout() { return opaquePipelineLayout; }

    const VkPipeline& getTransparentPipeline() { return transparentPipeline; }
    const VkPipelineLayout& getTransparentPipelineLayout() { return transparentPipelineLayout; }

private:
    VkRenderPass renderPass;

    VkDescriptorSetLayout descriptorSetLayout;

    VkPipelineLayout opaquePipelineLayout;
    VkPipeline opaquePipeline;

    VkPipelineLayout transparentPipelineLayout;
    VkPipeline transparentPipeline;
};

#endif // VULKAN_PIPELINE_H
