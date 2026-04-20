#include "graphics/buffer_manager.h"

#include <string.h>

#include "graphics/renderer.h"
#include "graphics/device.h"

std::vector<CopyInfo> BufferManager::pendingCopy;

void BufferManager::configure(Device& p_device, Renderer& p_renderer) {
    _device = &p_device;
    _renderer = &p_renderer;
}

void BufferManager::createBuffers(const GpuAllocatorConfig& p_gpu_allocator_config) {
    _opaque_allocator.init(*_device, p_gpu_allocator_config);
    _transparent_allocator.init(*_device, p_gpu_allocator_config);

    VkDeviceSize bufferSize = 1;

    voxelBuffer.createBuffer(bufferSize,
                            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                            *_device
    );

    updateVoxelBuffer.createBuffer(bufferSize,
                                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                *_device
    );
}

void BufferManager::createUniformBuffers(uint32_t p_frames_in_flight) {
    uniformBuffers.resize(p_frames_in_flight);

    for (size_t i = 0; i < p_frames_in_flight; i++) {
        uniformBuffers[i].createUniformBuffer(*_device);
    }
}

void BufferManager::updateUniformBuffer(uint32_t p_current_frame, glm::vec3 camPos, glm::mat4 matrix, glm::vec3 sunPos, glm::vec3 moonPos) {
    uniformBuffers.at(p_current_frame).updateUniformBuffer(camPos, matrix, sunPos, moonPos);
}

void BufferManager::applyCopies() {
    if (pendingCopy.size() <= 0) return;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(_renderer->getCopyCommandBuffer(), &beginInfo);

    for (CopyInfo infos : pendingCopy) {
        VkBufferCopy copyRegion {};
        copyRegion.size = infos.size;
        copyRegion.srcOffset = infos.srcOffset;
        copyRegion.dstOffset = infos.dstOffset;
        vkCmdCopyBuffer(_renderer->getCopyCommandBuffer(), infos.srcBuffer, infos.dstBuffer, 1, &copyRegion);
    }

    pendingCopy.clear();

    vkEndCommandBuffer(_renderer->getCopyCommandBuffer());

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_renderer->getCopyCommandBuffer();

    vkQueueSubmit(_device->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(_device->getGraphicsQueue());

    _renderer->resetCopyCommandBuffer();
    _opaque_allocator.resetStagingOffset();
    _transparent_allocator.resetStagingOffset();
}

void BufferManager::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, Device& p_device) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(p_device.getDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(p_device.getDevice(), image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, p_device);

    if (vkAllocateMemory(p_device.getDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(p_device.getDevice(), image, imageMemory, 0);
}

void BufferManager::copyBuffer(Buffer& srcBuffer, Buffer& dstBuffer, VkDeviceSize size) {
    copyBuffer(srcBuffer, dstBuffer, size, 0, 0);
}

void BufferManager::copyBuffer(Buffer& srcBuffer, Buffer& dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset) {
    if (size <= 0 || size+dstOffset > dstBuffer.getSize() || size+srcOffset > srcBuffer.getSize()) {
        throw std::runtime_error("BufferManager::copyBuffer() -> GPU buffer overflow !");
    }
    
    pendingCopy.push_back({
        srcBuffer.getBuffer(),
        dstBuffer.getBuffer(),
        size,
        srcOffset,
        dstOffset
    });
}

VkCommandBuffer BufferManager::beginSingleTimeCommands(Renderer& p_renderer, Device& p_device) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = p_renderer.getCommandPool();
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(p_device.getDevice(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void BufferManager::endSingleTimeCommands(VkCommandBuffer commandBuffer, Renderer& p_renderer, Device& p_device) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(p_device.getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(p_device.getGraphicsQueue());

    vkFreeCommandBuffers(p_device.getDevice(), p_renderer.getCommandPool(), 1, &commandBuffer);
}

uint32_t BufferManager::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, Device& p_device) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(p_device.getPhysicalDevice(), &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void BufferManager::cleanupBuffers() {
    _opaque_allocator.cleanup();
    _transparent_allocator.cleanup();
    voxelBuffer.cleanup();
    updateVoxelBuffer.cleanup();
}

void BufferManager::cleanupUniformBuffer() {
    for (auto& ubo : uniformBuffers) {
        ubo.cleanup();
    }
}
