#include "graphics/app.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <array>
#include <optional>
#include <set>

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include "world/chunk_manager.h"

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

void VulkanApp::initVulkan() {
    instance.createInstance();
    instance.setupDebugMessenger();
    instance.createSurface(window);

    device.pickPhysicalDevice(instance, swapchain);
    device.createLogicalDevice(instance);

    bufferManager.configure(device, renderer);

    swapchain.createSwapChain(window, instance, device);
    swapchain.createImageViews(device);

    graphicPipeline.createRenderPass(swapchain, device);
    graphicPipeline.createDescriptorSetLayout(device);
    graphicPipeline.createGraphicsPipeline(device);

    computePipeline.createDescriptorSetLayout(device);
    computePipeline.createComputePipeline(graphicPipeline, device);

    renderer.createCommandPool(device, instance);
    device.createDepthResources(swapchain);
    renderer.createFramebuffers(graphicPipeline, swapchain, device);

    texture.createTextureImage(renderer, device);
    texture.createTextureImageView(swapchain, device);
    texture.createTextureSampler(device);

    bufferManager.createBuffers();
    bufferManager.createUniformBuffers(swapchain.getFramesInFlight());

    descriptor.createDescriptorPool(device, swapchain.getFramesInFlight());
    descriptor.createDescriptorSets(
        bufferManager,
        texture,
        graphicPipeline,
        computePipeline,
        device,
        swapchain.getFramesInFlight()
    );

    renderer.createCommandBuffers(device, swapchain.getFramesInFlight());
    renderer.createComputeCommandBuffers(device, swapchain.getFramesInFlight());
    renderer.createSyncObjects(device, swapchain.getFramesInFlight());
} 

void VulkanApp::render() {
    bufferManager.applyCopies();
    updateDeltaTime();
    glfwPollEvents();
    camera.update(deltaTime);
    drawFrame();
}

void VulkanApp::init(glm::vec3 posCamera, float fov) {
    generated = 0;
    camera = Camera(posCamera, fov, 0);

    initWindow();
    initVulkan();

    camera.updateProjection(swapchain.getAspectRatio());
}

void VulkanApp::cleanup() {
    vkDeviceWaitIdle(device.getDevice());

    swapchain.cleanup(device);
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
        swapchain.recreateSwapChain(window, instance, graphicPipeline, renderer, device);
        renderer.resetCommandBuffers();
        camera.updateProjection(swapchain.getAspectRatio());
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    bufferManager.updateUniformBuffer(
        renderer.getCurrentFrame(),
        camera.getPosition(),
        camera.getProjectionMatrix()*camera.getViewMatrix(),
        {1, -1, 1},
        {1, -1, 1}
    );

    vkResetFences(device.getDevice(), 1, &renderer.getCurrentInFlightFences());


    if (!renderer.getCurrentCommandBuffersState()) {
        recordCommandBuffer(imageIndex);
        renderer.setCurrentCommandBuffersState(true);
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = waitSemaphores.size();
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitStages.data();

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &renderer.getCurrentCommandBuffers();

    VkSemaphore signalSemaphores[] = {renderer.getCurrentRenderFinishedSemaphores()};
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
        swapchain.recreateSwapChain(window, instance, graphicPipeline, renderer, device);
        renderer.resetCommandBuffers();
        camera.updateProjection(swapchain.getAspectRatio());
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    renderer.incrementeCurrentFrame();
}

void VulkanApp::recordCommandBuffer(uint32_t imageIndex) {
    auto command = renderer.getCurrentCommandBuffers();

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
        vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicPipeline.getOpaquePipelineLayout(), 0, 1, &(descriptor.getDescriptorSets())[renderer.getCurrentFrame()], 0, nullptr);

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
        vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicPipeline.getTransparentPipelineLayout(), 0, 1, &descriptor.getDescriptorSets()[renderer.getCurrentFrame()], 0, nullptr);

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
