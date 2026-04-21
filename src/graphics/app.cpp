#include "graphics/app.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <filesystem>
#include <stdexcept>
#include <vector>
#include <cstdint>
#include <array>

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

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
    validateRequiredResourcePath(resources.meshingComputeShader, "graphicsResources.meshingComputeShader");
}

} // namespace

void VulkanApp::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(1280, 720, "Minecraft-Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void VulkanApp::initVulkan(const GraphicsResourceConfig& resources, const GpuAllocatorConfig& gpu_allocator_config, uint32_t p_frames_in_flight) {
    instance.createInstance();
    instance.setupDebugMessenger();
    instance.createSurface(window);

    device.pickPhysicalDevice(instance, swapchain);
    device.createLogicalDevice(instance);

    bufferManager.configure(device, renderer);

    if (p_frames_in_flight == 0) {
        throw std::runtime_error("VulkanApp::initVulkan() -> framesInFlight must be greater than 0");
    }

    const uint32_t framesInFlight = p_frames_in_flight;
    swapchain.createSwapChain(window, instance, device, framesInFlight);
    swapchain.createImageViews(device);

    graphicPipeline.createRenderPass(swapchain, device);
    graphicPipeline.createDescriptorSetLayout(device);
    graphicPipeline.createGraphicsPipeline(
        resources.voxelVertexShader,
        resources.voxelFragmentShader,
        device
    );

    computePipeline.createDescriptorSetLayout(device);
    computePipeline.createComputePipeline(resources.meshingComputeShader, graphicPipeline, device);

    renderer.createCommandPool(device, instance);
    device.createDepthResources(swapchain);
    renderer.createFramebuffers(graphicPipeline, swapchain, device);

    texture.createTextureImage(resources.terrainTexture, renderer, device);
    texture.createTextureImageView(swapchain, device);
    texture.createTextureSampler(device);

    bufferManager.createBuffers(gpu_allocator_config);
    bufferManager.createUniformBuffers(swapchain.getImageCount());

    descriptor.createDescriptorPool(device, swapchain.getImageCount(), framesInFlight);
    descriptor.createDescriptorSets(
        bufferManager,
        texture,
        graphicPipeline,
        computePipeline,
        device,
        swapchain.getImageCount(),
        framesInFlight
    );

    renderer.createCommandBuffers(device, swapchain.getImageCount());
    renderer.createComputeCommandBuffers(device, framesInFlight);
    renderer.createSyncObjects(device, framesInFlight);
} 

void VulkanApp::recreateSwapchainResources() {
    swapchain.recreateSwapChain(window, instance, graphicPipeline, renderer, device);

    bufferManager.cleanupUniformBuffer();
    bufferManager.createUniformBuffers(swapchain.getImageCount());

    descriptor.cleanup(device);
    descriptor.createDescriptorPool(device, swapchain.getImageCount(), renderer.getFramesInFlight());
    descriptor.createDescriptorSets(
        bufferManager,
        texture,
        graphicPipeline,
        computePipeline,
        device,
        swapchain.getImageCount(),
        renderer.getFramesInFlight()
    );

    renderer.createCommandBuffers(device, swapchain.getImageCount());
    renderer.invalidateAllCommandBuffers();
    _last_opaque_indirect_count = bufferManager.getAllocator().getIndirectCount();
    _last_transparent_indirect_count = bufferManager.getTransparentAllocator().getIndirectCount();
    camera.updateProjection(swapchain.getAspectRatio());
}

void VulkanApp::render() {
    bufferManager.applyCopies();
    updateDeltaTime();
    glfwPollEvents();
    camera.update(deltaTime);
    drawFrame();
}

void VulkanApp::init(const VoxelEngineInitConfig& config) {
    generated = 0;
    validateGraphicsResources(config.graphicsResources);
    camera = Camera(config.cameraPos, config.fov, 0);

    initWindow();
    initVulkan(config.graphicsResources, config.gpuAllocator, config.framesInFlight);
    _last_opaque_indirect_count = bufferManager.getAllocator().getIndirectCount();
    _last_transparent_indirect_count = bufferManager.getTransparentAllocator().getIndirectCount();

    camera.updateProjection(swapchain.getAspectRatio());
}

void VulkanApp::cleanup() {
    vkDeviceWaitIdle(device.getDevice());

    swapchain.cleanup(device);
    device.cleanupDepthResources();
    graphicPipeline.cleanup(device);
    computePipeline.cleanup(device);
    bufferManager.cleanupUniformBuffer();
    descriptor.cleanup(device);
    texture.cleanup(device);
    graphicPipeline.cleanupDescriptorSetLayout(device);
    computePipeline.cleanupDescriptorSetLayout(device);
    chunkRenderStateCache.cleanup(bufferManager);
    bufferManager.cleanupBuffers();
    renderer.cleanup(device);
    device.cleanup();
    instance.cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();
}

void VulkanApp::drawFrame() {
    const uint32_t opaqueIndirectCount = bufferManager.getAllocator().getIndirectCount();
    const uint32_t transparentIndirectCount = bufferManager.getTransparentAllocator().getIndirectCount();
    if (opaqueIndirectCount != _last_opaque_indirect_count ||
        transparentIndirectCount != _last_transparent_indirect_count) {
        renderer.invalidateAllCommandBuffers();
        _last_opaque_indirect_count = opaqueIndirectCount;
        _last_transparent_indirect_count = transparentIndirectCount;
    }

    std::vector<VkSemaphore> waitSemaphores = {renderer.getCurrentImageAvailableSemaphores()};
    std::vector<VkPipelineStageFlags> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    vkWaitForFences(
        device.getDevice(),
        1,
        &renderer.getCurrentInFlightFences(),
        VK_TRUE,
        UINT64_MAX
    );

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        device.getDevice(),
        swapchain.getSwapChain(),
        UINT64_MAX,
        renderer.getCurrentImageAvailableSemaphores(),
        VK_NULL_HANDLE,
        &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchainResources();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    bufferManager.updateUniformBuffer(
        imageIndex,
        camera.getPosition(),
        camera.getProjectionMatrix()*camera.getViewMatrix(),
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
    submitInfo.waitSemaphoreCount = waitSemaphores.size();
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitStages.data();

    submitInfo.commandBufferCount = 1;
    const VkCommandBuffer& commandBuffer = renderer.getCommandBuffer(imageIndex);
    submitInfo.pCommandBuffers = &commandBuffer;

    VkSemaphore signalSemaphores[] = {renderer.getCurrentRenderFinishedSemaphore()};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(device.getGraphicsQueue(), 1, &submitInfo, renderer.getCurrentInFlightFences()) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
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

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapchainResources();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    renderer.incrementeCurrentFrame();
}

void VulkanApp::recordCommandBuffer(uint32_t imageIndex) {
    auto command = renderer.getCommandBuffer(imageIndex);

    vkResetCommandBuffer(command, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(command, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = graphicPipeline.getRenderPass();
    renderPassInfo.framebuffer = swapchain.getSwapChainFramebuffers()[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchain.getSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 1.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(command, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) swapchain.getSwapChainExtent().width;
        viewport.height = (float) swapchain.getSwapChainExtent().height;
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
        vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicPipeline.getOpaquePipelineLayout(), 0, 1, &(descriptor.getDescriptorSets())[imageIndex], 0, nullptr);

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
        vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicPipeline.getTransparentPipelineLayout(), 0, 1, &descriptor.getDescriptorSets()[imageIndex], 0, nullptr);

        vkCmdDrawIndexedIndirect(command,
            bufferManager.getTransparentAllocator().getIndirectBuffer().getBuffer(),
            0,
            bufferManager.getTransparentAllocator().getIndirectCount(),
            sizeof(DrawIndirectCommand)
        );

    vkCmdEndRenderPass(command);

    if (vkEndCommandBuffer(command) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void VulkanApp::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    VulkanApp* app = static_cast<VulkanApp*>(glfwGetWindowUserPointer(window));

    if (!app) return;

    int code = app->camera.processKeyboard(key, action);

    if (!code) return;

    switch (code)
    {
    case 1:
        glfwSetWindowShouldClose(window, true);
        break;

    default:
        break;
    }

}

void VulkanApp::mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    VulkanApp* app = static_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
    if (app) {
        double width = app->swapchain.getSwapChainExtent().width;
        double height = app->swapchain.getSwapChainExtent().height;

        app->camera.processMouse(width/2-xpos, height/2-ypos);

        glfwSetCursorPos(window, width/2, height/2);
    }
}

void VulkanApp::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

void VulkanApp::updateDeltaTime() {
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
}
