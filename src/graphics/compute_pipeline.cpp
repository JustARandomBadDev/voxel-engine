#include "graphics/compute_pipeline.h"

#include <array>

#include "graphics/device.h"
#include "graphics/descriptor.h"
#include "graphics/graphic_pipeline.h"
#include "graphics/renderer.h"


void ComputePipeline::createDescriptorSetLayout(Device& p_device) {
    VkDescriptorSetLayoutBinding voxelBinding{};
    voxelBinding.binding = 0;
    voxelBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    voxelBinding.descriptorCount = 1;
    voxelBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding updateBinding{};
    updateBinding.binding = 1;
    updateBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    updateBinding.descriptorCount = 1;
    updateBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding vertexBinding{};
    vertexBinding.binding = 2;
    vertexBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    vertexBinding.descriptorCount = 1;
    vertexBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding indexBinding{};
    indexBinding.binding = 3;
    indexBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    indexBinding.descriptorCount = 1;
    indexBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;


    std::array<VkDescriptorSetLayoutBinding, 4> bindings = {voxelBinding, updateBinding, vertexBinding, indexBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(p_device.getDevice(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute descriptor set layout!");
    }
}

void ComputePipeline::createComputePipeline(
    const std::filesystem::path& compute_shader_path,
    GraphicPipeline& p_graphic_pipeline,
    Device& p_device
) {
    VkShaderModule computeShaderModule = p_graphic_pipeline.createShaderModule(
        p_graphic_pipeline.readFile(compute_shader_path),
        p_device
    );

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(uint32_t);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &descriptorSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;
    vkCreatePipelineLayout(p_device.getDevice(), &layoutInfo, nullptr, &pipelineLayout);

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = createShaderStage(computeShaderModule, VK_SHADER_STAGE_COMPUTE_BIT);
    pipelineInfo.layout = pipelineLayout;
    vkCreateComputePipelines(p_device.getDevice(), nullptr, 1, &pipelineInfo, nullptr, &computePipeline);

    vkDestroyShaderModule(p_device.getDevice(), computeShaderModule, nullptr);
}

void ComputePipeline::dispatchCompute(Descriptor& p_descriptor, Renderer& p_renderer, Device& p_device) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    if (vkBeginCommandBuffer(p_renderer.getCurrentComputeCommandBuffers(), &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer for compute!");
    }

    vkCmdBindPipeline(p_renderer.getCurrentComputeCommandBuffers(), VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);

    vkCmdBindDescriptorSets(p_renderer.getCurrentComputeCommandBuffers(), VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &p_descriptor.getComputeDescriptorSets(p_renderer.getCurrentFrame()), 0, nullptr);

    vkCmdDispatch(p_renderer.getCurrentComputeCommandBuffers(), 1, 1, 1);

    if (vkEndCommandBuffer(p_renderer.getCurrentComputeCommandBuffers()) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer for compute!");
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &p_renderer.getCurrentComputeCommandBuffers();

    if (vkQueueSubmit(p_device.getComputeQueue(), 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit compute command buffer!");
    }

    vkQueueWaitIdle(p_device.getComputeQueue());
}

void ComputePipeline::cleanup(Device& p_device) {
    vkDestroyPipeline(p_device.getDevice(), computePipeline, nullptr);
    vkDestroyPipelineLayout(p_device.getDevice(), pipelineLayout, nullptr);
}

void ComputePipeline::cleanupDescriptorSetLayout(Device& p_device) {
    vkDestroyDescriptorSetLayout(p_device.getDevice(), descriptorSetLayout, nullptr);
}

VkPipelineShaderStageCreateInfo ComputePipeline::createShaderStage(VkShaderModule shaderModule, VkShaderStageFlagBits stage) {
    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = stage;
    shaderStageInfo.module = shaderModule;
    shaderStageInfo.pName = "main";

    return shaderStageInfo;
}
