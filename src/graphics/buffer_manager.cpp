#include "graphics/buffer_manager.h"

#include <sstream>
#include <stdexcept>
#include <string.h>

#include "graphics/device.h"
#include "graphics/renderer.h"

void BufferManager::ensureConfigured(const char* p_caller) const {
    if (_configured && _device != nullptr && _renderer != nullptr) return;

    std::ostringstream oss;
    oss << "BufferManager::" << p_caller
        << " -> BufferManager must be configured with a Device and Renderer before use";
    throw std::runtime_error(oss.str());
}

void BufferManager::ensureValidManagedBufferId(uint32_t p_buffer_id, const char* p_caller) const {
    if (p_buffer_id < _managed_buffers.size()) return;

    std::ostringstream oss;
    oss << "BufferManager::" << p_caller
        << " -> invalid managed buffer id: " << p_buffer_id;
    throw std::runtime_error(oss.str());
}

// BufferManager borrows Device and Renderer for later buffer creation and upload submission, and must be configured before use.
void BufferManager::configure(Device& p_device, Renderer& p_renderer) {
    _device = &p_device;
    _renderer = &p_renderer;
    _configured = true;
}

uint32_t BufferManager::createManagedBuffer(VkDeviceSize p_size, VkBufferUsageFlags p_usage, VkMemoryPropertyFlags p_properties) {
    ensureConfigured("createManagedBuffer()");

    Buffer buffer;
    buffer.createBuffer(p_size, p_usage, p_properties, *_device);
    _managed_buffers.push_back(std::move(buffer));
    return static_cast<uint32_t>(_managed_buffers.size() - 1);
}

Buffer& BufferManager::getManagedBuffer(uint32_t p_buffer_id) {
    ensureConfigured("getManagedBuffer()");
    ensureValidManagedBufferId(p_buffer_id, "getManagedBuffer()");
    return _managed_buffers[p_buffer_id];
}

const Buffer& BufferManager::getManagedBuffer(uint32_t p_buffer_id) const {
    ensureConfigured("getManagedBuffer() const");
    ensureValidManagedBufferId(p_buffer_id, "getManagedBuffer() const");
    return _managed_buffers[p_buffer_id];
}

// Resets allocator-backed storage, staging state, and pending uploads, then recreates persistent opaque/transparent mesh buffers.
void BufferManager::createBuffers(const GpuAllocatorConfig& p_gpu_allocator_config) {
    ensureConfigured("createBuffers()");

    _opaque_allocator.cleanup();
    _transparent_allocator.cleanup();

    for (Buffer& buffer : _managed_buffers) {
        buffer.cleanup();
    }

    if (_staging.getBuffer() != VK_NULL_HANDLE) {
        _staging.cleanup();
    }

    _managed_buffers.clear();
    _pending_uploads.clear();

    _staging.createBuffer(
        p_gpu_allocator_config.stagingBufferBytes,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        *_device
    );

    _opaque_allocator.init(*this, p_gpu_allocator_config);
    _transparent_allocator.init(*this, p_gpu_allocator_config);
}

void BufferManager::createUniformBuffers(uint32_t p_frames_in_flight) {
    ensureConfigured("createUniformBuffers()");

    if (!uniformBuffers.empty()) {
        cleanupUniformBuffer();
    }

    uniformBuffers.resize(p_frames_in_flight);

    for (size_t i = 0; i < p_frames_in_flight; i++) {
        uniformBuffers[i].createUniformBuffer(*_device);
    }
}

void BufferManager::updateUniformBuffer(uint32_t p_current_frame, glm::vec3 camPos, glm::mat4 matrix, glm::vec3 sunPos, glm::vec3 moonPos) {
    uniformBuffers.at(p_current_frame).updateUniformBuffer(camPos, matrix, sunPos, moonPos);
}

// Uploads are deferred and copy source bytes immediately so caller-owned mesh memory can be released or changed afterward.
void BufferManager::enqueueUpload(uint32_t p_dst_buffer_id, VkDeviceSize p_dst_offset, const void* p_data, VkDeviceSize p_size) {
    ensureConfigured("enqueueUpload()");
    ensureValidManagedBufferId(p_dst_buffer_id, "enqueueUpload()");

    if (p_data == nullptr) {
        throw std::runtime_error("BufferManager::enqueueUpload() -> source data pointer must not be null");
    }

    if (p_size == 0) return;

    const Buffer& dstBuffer = _managed_buffers[p_dst_buffer_id];
    if (p_dst_offset + p_size > dstBuffer.getSize()) {
        std::ostringstream oss;
        oss << "BufferManager::enqueueUpload() -> destination buffer overflow: "
            << "bufferId=" << p_dst_buffer_id
            << ", dstOffset=" << p_dst_offset
            << " bytes, size=" << p_size
            << " bytes, bufferCapacity=" << dstBuffer.getSize()
            << " bytes";
        throw std::runtime_error(oss.str());
    }

    if (p_size > _staging.getSize()) {
        std::ostringstream oss;
        oss << "BufferManager::enqueueUpload() -> upload too large for staging buffer: "
            << "requested=" << p_size
            << " bytes, stagingCapacity=" << _staging.getSize()
            << " bytes";
        throw std::runtime_error(oss.str());
    }

    PendingUpload upload;
    upload.dstBufferId = p_dst_buffer_id;
    upload.dstOffset = p_dst_offset;
    upload.data.resize(static_cast<size_t>(p_size));
    memcpy(upload.data.data(), p_data, static_cast<size_t>(p_size));
    _pending_uploads.push_back(std::move(upload));
}

// Uploads are batched into the shared staging buffer until it fills; any remaining uploads stay queued for a later submit pass.
void BufferManager::applyCopies() {
    if (_pending_uploads.empty()) return;

    ensureConfigured("applyCopies()");

    while (!_pending_uploads.empty()) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(_renderer->getCopyCommandBuffer(), &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("BufferManager::applyCopies() -> failed to begin copy command buffer recording");
        }

        VkDeviceSize stagingOffset = 0;
        size_t processedUploads = 0;

        for (const PendingUpload& upload : _pending_uploads) {
            const VkDeviceSize uploadSize = static_cast<VkDeviceSize>(upload.data.size());
            if (stagingOffset + uploadSize > _staging.getSize()) {
                break;
            }

            void* mappedData = nullptr;
            const VkResult mapResult = vkMapMemory(
                _device->getDevice(),
                _staging.getBufferMemory(),
                stagingOffset,
                uploadSize,
                0,
                &mappedData
            );

            if (mapResult != VK_SUCCESS) {
                throw std::runtime_error("BufferManager::applyCopies() -> failed to map staging buffer memory");
            }

            memcpy(mappedData, upload.data.data(), static_cast<size_t>(uploadSize));
            vkUnmapMemory(_device->getDevice(), _staging.getBufferMemory());

            VkBufferCopy copyRegion{};
            copyRegion.size = uploadSize;
            copyRegion.srcOffset = stagingOffset;
            copyRegion.dstOffset = upload.dstOffset;
            vkCmdCopyBuffer(
                _renderer->getCopyCommandBuffer(),
                _staging.getBuffer(),
                getManagedBuffer(upload.dstBufferId).getBuffer(),
                1,
                &copyRegion
            );

            stagingOffset += uploadSize;
            processedUploads++;
        }

        if (processedUploads == 0) {
            throw std::runtime_error("BufferManager::applyCopies() -> no pending upload fit in staging buffer");
        }

        if (vkEndCommandBuffer(_renderer->getCopyCommandBuffer()) != VK_SUCCESS) {
            throw std::runtime_error("BufferManager::applyCopies() -> failed to end copy command buffer recording");
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &_renderer->getCopyCommandBuffer();

        if (vkQueueSubmit(_device->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
            throw std::runtime_error("BufferManager::applyCopies() -> failed to submit copy command buffer");
        }

        vkQueueWaitIdle(_device->getGraphicsQueue());
        _renderer->resetCopyCommandBuffer();
        _pending_uploads.erase(_pending_uploads.begin(), _pending_uploads.begin() + static_cast<long>(processedUploads));
    }
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
        throw std::runtime_error(
            "BufferManager::createImage() -> failed to create image (width: "
            + std::to_string(width) + ", height: " + std::to_string(height) + ")"
        );
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(p_device.getDevice(), image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, p_device);

    if (vkAllocateMemory(p_device.getDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error(
            "BufferManager::createImage() -> failed to allocate image memory (size: "
            + std::to_string(memRequirements.size) + " bytes)"
        );
    }

    if (vkBindImageMemory(p_device.getDevice(), image, imageMemory, 0) != VK_SUCCESS) {
        throw std::runtime_error("BufferManager::createImage() -> failed to bind image memory");
    }
}

// These helpers allocate, submit, wait, and free one-shot command buffers from the renderer command pool for transient setup work.
VkCommandBuffer BufferManager::beginSingleTimeCommands(Renderer& p_renderer, Device& p_device) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = p_renderer.getCommandPool();
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(p_device.getDevice(), &allocInfo, &commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("BufferManager::beginSingleTimeCommands() -> failed to allocate single-use command buffer");
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("BufferManager::beginSingleTimeCommands() -> failed to begin single-use command buffer recording");
    }

    return commandBuffer;
}

void BufferManager::endSingleTimeCommands(VkCommandBuffer commandBuffer, Renderer& p_renderer, Device& p_device) {
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("BufferManager::endSingleTimeCommands() -> failed to end single-use command buffer recording");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (vkQueueSubmit(p_device.getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("BufferManager::endSingleTimeCommands() -> failed to submit single-use command buffer");
    }
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

    throw std::runtime_error("BufferManager::findMemoryType() -> failed to find a suitable Vulkan memory type");
}

// Releases allocator-managed mesh storage, staging state, and pending uploads, but leaves per-frame uniform buffers untouched.
void BufferManager::cleanupBuffers() {
    _pending_uploads.clear();
    _opaque_allocator.cleanup();
    _transparent_allocator.cleanup();

    for (Buffer& buffer : _managed_buffers) {
        buffer.cleanup();
    }
    _managed_buffers.clear();

    _staging.cleanup();
}

void BufferManager::cleanupUniformBuffer() {
    for (auto& ubo : uniformBuffers) {
        ubo.cleanup();
    }

    uniformBuffers.clear();
}
