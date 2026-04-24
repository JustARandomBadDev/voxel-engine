#include "graphics/renderer.h"

#include <stdexcept>

#include "graphics/device.h"
#include "graphics/instance.h"

// Graphics draw commands and transient upload/copy commands are allocated from the graphics+compute command pool.
void Renderer::createCommandPool(Device& p_device, Instance& p_instance) {
    QueueFamilyIndices queueFamilyIndices = p_device.findQueueFamilies(p_device.getPhysicalDevice(), p_instance);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily.value();

    if (vkCreateCommandPool(p_device.getDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics command pool!");
    }
}

// Per-image draw command buffers track the current swapchain image count,
// while the copy command buffer is allocated once and reused by upload helpers.
void Renderer::createCommandBuffers(Device& p_device, uint32_t p_image_count) {
    if (!commandBuffers.empty()) {
        vkFreeCommandBuffers(
            p_device.getDevice(),
            commandPool,
            static_cast<uint32_t>(commandBuffers.size()),
            commandBuffers.data()
        );
        commandBuffers.clear();
    }

    swapchainImageCount = p_image_count;
    commandBuffers.resize(swapchainImageCount);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(p_device.getDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    commandBufferDirty.assign(swapchainImageCount, true);
    currentFrame = framesInFlight == 0 ? 0 : (currentFrame % framesInFlight);

    if (copyCommandBuffer == VK_NULL_HANDLE) {
        VkCommandBufferAllocateInfo copyAllocInfo{};
        copyAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        copyAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        copyAllocInfo.commandPool = commandPool;
        copyAllocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(p_device.getDevice(), &copyAllocInfo, &copyCommandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }
}

// Acquire synchronization is sized by frames-in-flight, while render-finished semaphores follow swapchain image count.
void Renderer::createSyncObjects(Device& p_device, uint32_t p_frames_in_flight, uint32_t p_image_count) {
    if (!imageAvailableSemaphores.empty()) {
        for (size_t i = 0; i < imageAvailableSemaphores.size(); i++) {
            vkDestroySemaphore(p_device.getDevice(), imageAvailableSemaphores[i], nullptr);
        }
        imageAvailableSemaphores.clear();
    }

    if (!renderFinishedSemaphores.empty()) {
        for (size_t i = 0; i < renderFinishedSemaphores.size(); i++) {
            vkDestroySemaphore(p_device.getDevice(), renderFinishedSemaphores[i], nullptr);
        }
        renderFinishedSemaphores.clear();
    }

    if (!inFlightFences.empty()) {
        for (size_t i = 0; i < inFlightFences.size(); i++) {
            vkDestroyFence(p_device.getDevice(), inFlightFences[i], nullptr);
        }
        inFlightFences.clear();
    }

    framesInFlight = p_frames_in_flight;
    swapchainImageCount = p_image_count;
    imageAvailableSemaphores.resize(framesInFlight);
    renderFinishedSemaphores.resize(swapchainImageCount);
    inFlightFences.resize(framesInFlight);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < framesInFlight; i++) {
        if (vkCreateSemaphore(p_device.getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(p_device.getDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }

    for (size_t i = 0; i < swapchainImageCount; i++) {
        if (vkCreateSemaphore(p_device.getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render-finished semaphores for swapchain images!");
        }
    }
}

// Command buffers are marked dirty when swapchain-dependent state or indirect draw state changes.
void Renderer::invalidateAllCommandBuffers() {
    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        commandBufferDirty[i] = true;
    }
}

void Renderer::resetCopyCommandBuffer() {
    vkResetCommandBuffer(copyCommandBuffer, 0);
}

// Renderer releases frame-execution sync objects and destroys the command pool, which releases its command buffers.
void Renderer::cleanup(Device& p_device) {
    for (size_t i = 0; i < framesInFlight; i++) {
        vkDestroySemaphore(p_device.getDevice(), imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(p_device.getDevice(), inFlightFences[i], nullptr);
    }

    for (size_t i = 0; i < swapchainImageCount; i++) {
        vkDestroySemaphore(p_device.getDevice(), renderFinishedSemaphores[i], nullptr);
    }

    vkDestroyCommandPool(p_device.getDevice(), commandPool, nullptr);
}

void Renderer::incrementeCurrentFrame() {
    currentFrame = (currentFrame + 1) % framesInFlight;
}
