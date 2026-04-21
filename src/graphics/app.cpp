#include "graphics/app.h"

#include <array>
#include <filesystem>
#include <stdexcept>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

namespace {

void validateRequiredResourcePath(const std::filesystem::path& path, const char* resource_name) {
    if (path.empty()) {
        throw std::runtime_error(std::string("missing required resource path: ") + resource_name);
    }

    if (!std::filesystem::exists(path)) {
        throw std::runtime_error(
            std::string("required resource does not exist: ") + resource_name + " -> " + path.string()
        );
    }

    if (!std::filesystem::is_regular_file(path)) {
        throw std::runtime_error(
            std::string("required resource is not a file: ") + resource_name + " -> " + path.string()
        );
    }
}

void validateGraphicsResources(const GraphicsResourceConfig& resources) {
    validateRequiredResourcePath(resources.terrainTexture, "graphicsResources.terrainTexture");
    validateRequiredResourcePath(resources.voxelVertexShader, "graphicsResources.voxelVertexShader");
    validateRequiredResourcePath(resources.voxelFragmentShader, "graphicsResources.voxelFragmentShader");
}

void validateInitConfig(const VoxelEngineInitConfig& config) {
    if (!config.vulkanHost.createSurface) {
        throw std::runtime_error("VulkanApp::init() -> vulkanHost.createSurface callback must be set");
    }

    if (!config.vulkanHost.getFramebufferExtent) {
        throw std::runtime_error("VulkanApp::init() -> vulkanHost.getFramebufferExtent callback must be set");
    }

    if (config.vulkanHost.requiredInstanceExtensions.empty()) {
        throw std::runtime_error("VulkanApp::init() -> vulkanHost.requiredInstanceExtensions must not be empty");
    }

    if (config.framesInFlight == 0) {
        throw std::runtime_error("VulkanApp::init() -> framesInFlight must be greater than 0");
    }
}

} // namespace

void VulkanApp::initVulkan(
    const GraphicsResourceConfig& resources,
    const GpuAllocatorConfig& gpu_allocator_config,
    uint32_t p_frames_in_flight,
    bool p_enable_validation_layers
) {
    instance.createInstance(p_enable_validation_layers, _host_config.requiredInstanceExtensions);
    instance.setupDebugMessenger();
    instance.createSurface(_host_config.createSurface);

    device.pickPhysicalDevice(instance, swapchain);
    device.createLogicalDevice(instance);

    bufferManager.configure(device, renderer);

    const VkExtent2D framebufferExtent = getFramebufferExtent();
    if (framebufferExtent.width == 0 || framebufferExtent.height == 0) {
        throw std::runtime_error("VulkanApp::initVulkan() -> host framebuffer extent must be non-zero during initialization");
    }

    const uint32_t framesInFlight = p_frames_in_flight;
    swapchain.createSwapChain(framebufferExtent, instance, device, framesInFlight);
    swapchain.createImageViews(device);

    graphicPipeline.createRenderPass(swapchain, device);
    graphicPipeline.createDescriptorSetLayout(device);
    graphicPipeline.createGraphicsPipeline(
        resources.voxelVertexShader,
        resources.voxelFragmentShader,
        device
    );

    renderer.createCommandPool(device, instance);
    device.createDepthResources(swapchain);
    renderer.createFramebuffers(graphicPipeline, swapchain, device);

    texture.createTextureImage(resources.terrainTexture, renderer, device);
    texture.createTextureImageView(swapchain, device);
    texture.createTextureSampler(device);

    bufferManager.createBuffers(gpu_allocator_config);
    bufferManager.createUniformBuffers(swapchain.getImageCount());

    descriptor.createDescriptorPool(device, swapchain.getImageCount());
    descriptor.createDescriptorSets(
        bufferManager,
        texture,
        graphicPipeline,
        device,
        swapchain.getImageCount()
    );

    renderer.createCommandBuffers(device, swapchain.getImageCount());
    renderer.createSyncObjects(device, framesInFlight, swapchain.getImageCount());
}

VkExtent2D VulkanApp::getFramebufferExtent() const {
    return _host_config.getFramebufferExtent();
}

float VulkanApp::getAspectRatio() const {
    const VkExtent2D framebufferExtent = getFramebufferExtent();
    if (framebufferExtent.width > 0 && framebufferExtent.height > 0) {
        return static_cast<float>(framebufferExtent.width) / static_cast<float>(framebufferExtent.height);
    }

    if (swapchain.getImageCount() > 0) {
        return swapchain.getAspectRatio();
    }

    return 1.0f;
}

bool VulkanApp::recreateSwapchainResources() {
    const VkExtent2D framebufferExtent = getFramebufferExtent();
    if (framebufferExtent.width == 0 || framebufferExtent.height == 0) {
        _swapchain_needs_recreate = true;
        return false;
    }

    swapchain.recreateSwapChain(framebufferExtent, instance, graphicPipeline, renderer, device);

    bufferManager.cleanupUniformBuffer();
    bufferManager.createUniformBuffers(swapchain.getImageCount());

    descriptor.cleanup(device);
    descriptor.createDescriptorPool(device, swapchain.getImageCount());
    descriptor.createDescriptorSets(
        bufferManager,
        texture,
        graphicPipeline,
        device,
        swapchain.getImageCount()
    );

    renderer.createCommandBuffers(device, swapchain.getImageCount());
    renderer.createSyncObjects(device, renderer.getFramesInFlight(), swapchain.getImageCount());
    renderer.invalidateAllCommandBuffers();
    _last_opaque_indirect_count = bufferManager.getAllocator().getIndirectCount();
    _last_transparent_indirect_count = bufferManager.getTransparentAllocator().getIndirectCount();
    _swapchain_needs_recreate = false;
    return true;
}

bool VulkanApp::syncSwapchainToHostExtent() {
    const VkExtent2D framebufferExtent = getFramebufferExtent();
    if (framebufferExtent.width == 0 || framebufferExtent.height == 0) {
        _swapchain_needs_recreate = true;
        return false;
    }

    const VkExtent2D swapchainExtent = swapchain.getSwapChainExtent();
    const bool hostExtentChanged =
        swapchain.getImageCount() > 0 &&
        (swapchainExtent.width != framebufferExtent.width || swapchainExtent.height != framebufferExtent.height);

    if (_swapchain_needs_recreate || hostExtentChanged) {
        return recreateSwapchainResources();
    }

    return true;
}

void VulkanApp::render(const Camera& camera) {
    if (!syncSwapchainToHostExtent()) return;

    bufferManager.applyCopies();
    drawFrame(camera);
}

void VulkanApp::init(const VoxelEngineInitConfig& config) {
    validateInitConfig(config);
    validateGraphicsResources(config.graphicsResources);
    _host_config = config.vulkanHost;
    _clear_color = config.clearColor;

    initVulkan(config.graphicsResources, config.gpuAllocator, config.framesInFlight, config.enableValidationLayers);
    _last_opaque_indirect_count = bufferManager.getAllocator().getIndirectCount();
    _last_transparent_indirect_count = bufferManager.getTransparentAllocator().getIndirectCount();
}

void VulkanApp::cleanup() {
    if (device.getDevice() != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device.getDevice());
    }

    swapchain.cleanup(device);
    device.cleanupDepthResources();
    graphicPipeline.cleanup(device);
    bufferManager.cleanupUniformBuffer();
    descriptor.cleanup(device);
    texture.cleanup(device);
    graphicPipeline.cleanupDescriptorSetLayout(device);
    chunkRenderStateCache.cleanup(bufferManager);
    bufferManager.cleanupBuffers();
    renderer.cleanup(device);
    device.cleanup();
    instance.cleanup();
}

void VulkanApp::drawFrame(const Camera& camera) {
    const uint32_t opaqueIndirectCount = bufferManager.getAllocator().getIndirectCount();
    const uint32_t transparentIndirectCount = bufferManager.getTransparentAllocator().getIndirectCount();
    if (opaqueIndirectCount != _last_opaque_indirect_count ||
        transparentIndirectCount != _last_transparent_indirect_count) {
        renderer.invalidateAllCommandBuffers();
        _last_opaque_indirect_count = opaqueIndirectCount;
        _last_transparent_indirect_count = transparentIndirectCount;
    }

    VkSemaphore waitSemaphores[] = {renderer.getCurrentImageAvailableSemaphores()};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    vkWaitForFences(
        device.getDevice(),
        1,
        &renderer.getCurrentInFlightFences(),
        VK_TRUE,
        UINT64_MAX
    );

    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(
        device.getDevice(),
        swapchain.getSwapChain(),
        UINT64_MAX,
        renderer.getCurrentImageAvailableSemaphores(),
        VK_NULL_HANDLE,
        &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        _swapchain_needs_recreate = true;
        recreateSwapchainResources();
        return;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("VulkanApp::drawFrame() -> failed to acquire swapchain image");
    }

    bufferManager.updateUniformBuffer(
        imageIndex,
        camera.getPosition(),
        camera.getProjectionMatrix() * camera.getViewMatrix(),
        {1, -1, 1},
        {1, -1, 1}
    );

    vkResetFences(device.getDevice(), 1, &renderer.getCurrentInFlightFences());

    if (renderer.isCommandBufferDirty(imageIndex)) {
        recordCommandBuffer(imageIndex);
        renderer.setCommandBufferDirty(imageIndex, false);
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    const VkCommandBuffer& commandBuffer = renderer.getCommandBuffer(imageIndex);
    submitInfo.pCommandBuffers = &commandBuffer;

    VkSemaphore signalSemaphores[] = {renderer.getRenderFinishedSemaphore(imageIndex)};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(device.getGraphicsQueue(), 1, &submitInfo, renderer.getCurrentInFlightFences()) != VK_SUCCESS) {
        throw std::runtime_error("VulkanApp::drawFrame() -> failed to submit draw command buffer");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapchain.getSwapChain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(device.getPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        _swapchain_needs_recreate = true;
        recreateSwapchainResources();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("VulkanApp::drawFrame() -> failed to present swapchain image");
    }

    renderer.incrementeCurrentFrame();
}

void VulkanApp::recordCommandBuffer(uint32_t imageIndex) {
    auto command = renderer.getCommandBuffer(imageIndex);

    vkResetCommandBuffer(command, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(command, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("VulkanApp::recordCommandBuffer() -> failed to begin command buffer recording");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = graphicPipeline.getRenderPass();
    renderPassInfo.framebuffer = swapchain.getSwapChainFramebuffers()[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchain.getSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{
        _clear_color.r,
        _clear_color.g,
        _clear_color.b,
        _clear_color.a
    }};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(command, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchain.getSwapChainExtent().width);
    viewport.height = static_cast<float>(swapchain.getSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchain.getSwapChainExtent();
    vkCmdSetScissor(command, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = {bufferManager.getVertexBuffers().getBuffer()};
    VkDeviceSize offsets[] = {0};

    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicPipeline.getOpaquePipeline());
    vkCmdBindVertexBuffers(command, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(command, bufferManager.getIndexBuffers().getBuffer(), 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(
        command,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphicPipeline.getOpaquePipelineLayout(),
        0,
        1,
        &(descriptor.getDescriptorSets())[imageIndex],
        0,
        nullptr
    );

    vkCmdDrawIndexedIndirect(
        command,
        bufferManager.getAllocator().getIndirectBuffer().getBuffer(),
        0,
        bufferManager.getAllocator().getIndirectCount(),
        sizeof(DrawIndirectCommand)
    );

    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicPipeline.getTransparentPipeline());
    VkDeviceSize offset = 0;

    vkCmdBindVertexBuffers(
        command,
        0,
        1,
        &bufferManager.getTransparentVertexBuffers().getBuffer(),
        &offset
    );
    vkCmdBindIndexBuffer(command, bufferManager.getTransparentIndexBuffers().getBuffer(), 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(
        command,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphicPipeline.getTransparentPipelineLayout(),
        0,
        1,
        &descriptor.getDescriptorSets()[imageIndex],
        0,
        nullptr
    );

    vkCmdDrawIndexedIndirect(
        command,
        bufferManager.getTransparentAllocator().getIndirectBuffer().getBuffer(),
        0,
        bufferManager.getTransparentAllocator().getIndirectCount(),
        sizeof(DrawIndirectCommand)
    );

    vkCmdEndRenderPass(command);

    if (vkEndCommandBuffer(command) != VK_SUCCESS) {
        throw std::runtime_error("VulkanApp::recordCommandBuffer() -> failed to record command buffer");
    }
}
