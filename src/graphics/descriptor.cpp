#include "graphics/descriptor.h"

#include <array>
#include <string>

#include "graphics/buffer.h"
#include "graphics/device.h"
#include "graphics/graphic_pipeline.h"
#include "graphics/texture.h"
#include "graphics/compute_pipeline.h"
#include "graphics/buffer_manager.h"
#include "graphics/uniform_buffer.h"

void Descriptor::createDescriptorPool(Device& p_device, uint32_t p_image_count, uint32_t p_frames_in_flight) {
    std::array<VkDescriptorPoolSize, 3> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[0].descriptorCount = p_frames_in_flight * 4;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount = p_image_count;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[2].descriptorCount = p_image_count;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = p_image_count + p_frames_in_flight;

    if (vkCreateDescriptorPool(p_device.getDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error(
            "Descriptor::createDescriptorPool() -> failed to create descriptor pool (imageCount: "
            + std::to_string(p_image_count) + ", framesInFlight: " + std::to_string(p_frames_in_flight) + ")"
        );
    }
}

void Descriptor::createDescriptorSets(BufferManager& p_buffer_manager, Texture& p_texture, GraphicPipeline& p_graphic_pipeline, ComputePipeline& p_compute_pipeline, Device& p_device, uint32_t p_image_count, uint32_t p_frames_in_flight) {
    std::vector<VkDescriptorSetLayout> layouts(p_image_count, p_graphic_pipeline.getDescriptorSetLayout());
    std::vector<VkDescriptorSetLayout> computeLayouts(p_frames_in_flight, p_compute_pipeline.getDescriptorSetLayout());

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = p_image_count;

    allocInfo.pSetLayouts = layouts.data();
    descriptorSets.resize(p_image_count);
    if (vkAllocateDescriptorSets(p_device.getDevice(), &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Descriptor::createDescriptorSets() -> failed to allocate graphics descriptor sets");
    }

    allocInfo.descriptorSetCount = p_frames_in_flight;
    allocInfo.pSetLayouts = computeLayouts.data();
    computeDescriptorSets.resize(p_frames_in_flight);
    if (vkAllocateDescriptorSets(p_device.getDevice(), &allocInfo, computeDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Descriptor::createDescriptorSets() -> failed to allocate compute descriptor sets");
    }

    for (size_t i = 0; i < p_image_count; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = p_buffer_manager.getUniformBuffer(i).getBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = p_texture.getTextureImageView();
        imageInfo.sampler = p_texture.getTextureSampler();

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(p_device.getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

        if (descriptorSets[i] == VK_NULL_HANDLE) {
            throw std::runtime_error(std::string("Descriptor set allocation failed for index: ")+std::to_string(i));
        }
    }

    for (size_t i = 0; i < p_frames_in_flight; i++) {
        VkDescriptorBufferInfo voxelBufferInfo{};
        voxelBufferInfo.buffer = p_buffer_manager.getVoxelBuffer().getBuffer();
        voxelBufferInfo.offset = 0;
        voxelBufferInfo.range = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo updateBufferInfo{};
        updateBufferInfo.buffer = p_buffer_manager.getUpdateVoxelBuffer().getBuffer();
        updateBufferInfo.offset = 0;
        updateBufferInfo.range = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo vertexBufferInfo{};
        vertexBufferInfo.buffer = p_buffer_manager.getVertexBuffers().getBuffer();
        vertexBufferInfo.offset = 0;
        vertexBufferInfo.range = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo indexBufferInfo{};
        indexBufferInfo.buffer = p_buffer_manager.getIndexBuffers().getBuffer();
        indexBufferInfo.offset = 0;
        indexBufferInfo.range = VK_WHOLE_SIZE;

        std::array<VkWriteDescriptorSet, 4> descriptorWritesCompute{};

        descriptorWritesCompute[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWritesCompute[0].dstSet = computeDescriptorSets[i];
        descriptorWritesCompute[0].dstBinding = 0;
        descriptorWritesCompute[0].dstArrayElement = 0;
        descriptorWritesCompute[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWritesCompute[0].descriptorCount = 1;
        descriptorWritesCompute[0].pBufferInfo = &voxelBufferInfo;

        descriptorWritesCompute[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWritesCompute[1].dstSet = computeDescriptorSets[i];
        descriptorWritesCompute[1].dstBinding = 1;
        descriptorWritesCompute[1].dstArrayElement = 0;
        descriptorWritesCompute[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWritesCompute[1].descriptorCount = 1;
        descriptorWritesCompute[1].pBufferInfo = &updateBufferInfo;

        descriptorWritesCompute[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWritesCompute[2].dstSet = computeDescriptorSets[i];
        descriptorWritesCompute[2].dstBinding = 2;
        descriptorWritesCompute[2].dstArrayElement = 0;
        descriptorWritesCompute[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWritesCompute[2].descriptorCount = 1;
        descriptorWritesCompute[2].pBufferInfo = &vertexBufferInfo;

        descriptorWritesCompute[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWritesCompute[3].dstSet = computeDescriptorSets[i];
        descriptorWritesCompute[3].dstBinding = 3;
        descriptorWritesCompute[3].dstArrayElement = 0;
        descriptorWritesCompute[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWritesCompute[3].descriptorCount = 1;
        descriptorWritesCompute[3].pBufferInfo = &indexBufferInfo;

        vkUpdateDescriptorSets(p_device.getDevice(), static_cast<uint32_t>(descriptorWritesCompute.size()), descriptorWritesCompute.data(), 0, nullptr);

        if (computeDescriptorSets[i] == VK_NULL_HANDLE) {
            throw std::runtime_error(std::string("Compute descriptor set allocation failed for index: ")+std::to_string(i));
        }
    }
}

void Descriptor::cleanup(Device& p_device) {
    vkDestroyDescriptorPool(p_device.getDevice(), descriptorPool, nullptr);
    descriptorSets.clear();
    computeDescriptorSets.clear();
}
