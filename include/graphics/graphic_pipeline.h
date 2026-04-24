#ifndef VULKAN_GRAPHIC_PIPELINE_H
#define VULKAN_GRAPHIC_PIPELINE_H

#include <filesystem>
#include <vulkan/vulkan.h>
#include <vector>

class Device;
class Swapchain;

// Owns the persistent descriptor set layout plus the current render pass and graphics pipelines for voxel rendering.
class GraphicPipeline {
public:
    void createRenderPass(Swapchain& p_swapchain, Device& p_device);
    void createDescriptorSetLayout(Device& p_device);
    void createGraphicsPipeline(
        const std::filesystem::path& vertex_shader_path,
        const std::filesystem::path& fragment_shader_path,
        Device& p_device
    );
    void cleanup(Device& p_device);
    void cleanupDescriptorSetLayout(Device& p_device);

    VkShaderModule createShaderModule(const std::vector<char>& code, Device& p_device);
    static std::vector<char> readFile(const std::filesystem::path& filename);

    VkRenderPass getRenderPass() const { return renderPass; }

    const VkDescriptorSetLayout& getDescriptorSetLayout() const { return descriptorSetLayout; }

    const VkPipeline& getOpaquePipeline() const { return opaquePipeline; }
    const VkPipelineLayout& getOpaquePipelineLayout() const { return opaquePipelineLayout; }

    const VkPipeline& getTransparentPipeline() const { return transparentPipeline; }
    const VkPipelineLayout& getTransparentPipelineLayout() const { return transparentPipelineLayout; }

private:
    VkRenderPass renderPass = VK_NULL_HANDLE;

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

    VkPipelineLayout opaquePipelineLayout = VK_NULL_HANDLE;
    VkPipeline opaquePipeline = VK_NULL_HANDLE;

    VkPipelineLayout transparentPipelineLayout = VK_NULL_HANDLE;
    VkPipeline transparentPipeline = VK_NULL_HANDLE;
};

#endif // VULKAN_PIPELINE_H
