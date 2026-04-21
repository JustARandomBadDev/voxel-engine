#include "graphics/descriptor.h"

#include <array>
#include <string>

#include "graphics/buffer.h"
#include "graphics/device.h"
#include "graphics/graphic_pipeline.h"
#include "graphics/texture.h"
#include "graphics/buffer_manager.h"
#include "graphics/uniform_buffer.h"

void Descriptor::createDescriptorPool(Device& p_device, uint32_t p_image_count) {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = p_image_count;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = p_image_count;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = p_image_count;

    if (vkCreateDescriptorPool(p_device.getDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error(
            "Descriptor::createDescriptorPool() -> failed to create descriptor pool (imageCount: "
            + std::to_string(p_image_count) + ")"
        );
    }
}

void Descriptor::createDescriptorSets(BufferManager& p_buffer_manager, Texture& p_texture, GraphicPipeline& p_graphic_pipeline, Device& p_device, uint32_t p_image_count) {
    std::vector<VkDescriptorSetLayout> layouts(p_image_count, p_graphic_pipeline.getDescriptorSetLayout());

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = p_image_count;

    allocInfo.pSetLayouts = layouts.data();
    descriptorSets.resize(p_image_count);
    if (vkAllocateDescriptorSets(p_device.getDevice(), &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Descriptor::createDescriptorSets() -> failed to allocate graphics descriptor sets");
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
}

void Descriptor::cleanup(Device& p_device) {
    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(p_device.getDevice(), descriptorPool, nullptr);
    }
    descriptorPool = VK_NULL_HANDLE;
    descriptorSets.clear();
}
