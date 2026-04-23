#include "graphics/frame_renderer.h"

#include <stdexcept>

#include "core/camera.h"
#include "graphics/buffer_manager.h"
#include "graphics/command_recorder.h"
#include "graphics/device.h"
#include "graphics/renderer.h"
#include "graphics/swapchain.h"

FrameRenderer::FrameRenderer(
    Device& p_device,
    Renderer& p_renderer,
    Swapchain& p_swapchain,
    BufferManager& p_buffer_manager,
    CommandRecorder& p_command_recorder
)
: _device(p_device),
  _renderer(p_renderer),
  _swapchain(p_swapchain),
  _buffer_manager(p_buffer_manager),
  _command_recorder(p_command_recorder) {}

void FrameRenderer::refreshIndirectCounts() {
    _last_opaque_indirect_count = _buffer_manager.getAllocator().getIndirectCount();
    _last_transparent_indirect_count = _buffer_manager.getTransparentAllocator().getIndirectCount();
}

void FrameRenderer::onFrameResourcesRecreated() {
    refreshIndirectCounts();
}

void FrameRenderer::syncCommandBufferRecordingState() {
    const uint32_t opaqueIndirectCount = _buffer_manager.getAllocator().getIndirectCount();
    const uint32_t transparentIndirectCount = _buffer_manager.getTransparentAllocator().getIndirectCount();
    if (opaqueIndirectCount != _last_opaque_indirect_count ||
        transparentIndirectCount != _last_transparent_indirect_count) {
        _renderer.invalidateAllCommandBuffers();
        refreshIndirectCounts();
    }
}

VkResult FrameRenderer::acquireFrameImage(uint32_t& p_image_index) {
    return vkAcquireNextImageKHR(
        _device.getDevice(),
        _swapchain.getSwapChain(),
        UINT64_MAX,
        _renderer.getCurrentImageAvailableSemaphores(),
        VK_NULL_HANDLE,
        &p_image_index
    );
}

void FrameRenderer::updateFrameResources(uint32_t p_image_index, const Camera& camera) {
    _buffer_manager.updateUniformBuffer(
        p_image_index,
        camera.getPosition(),
        camera.getProjectionMatrix() * camera.getViewMatrix(),
        {1, -1, 1},
        {1, -1, 1}
    );
}

void FrameRenderer::ensureFrameCommandBufferRecorded(uint32_t p_image_index, const glm::vec4& p_clear_color) {
    if (_renderer.isCommandBufferDirty(p_image_index)) {
        _command_recorder.record(p_image_index, p_clear_color);
        _renderer.setCommandBufferDirty(p_image_index, false);
    }
}

void FrameRenderer::submitFrame(uint32_t p_image_index) {
    VkSemaphore waitSemaphores[] = {_renderer.getCurrentImageAvailableSemaphores()};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {_renderer.getRenderFinishedSemaphore(p_image_index)};

    vkResetFences(_device.getDevice(), 1, &_renderer.getCurrentInFlightFences());

    const VkCommandBuffer& commandBuffer = _renderer.getCommandBuffer(p_image_index);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(_device.getGraphicsQueue(), 1, &submitInfo, _renderer.getCurrentInFlightFences()) != VK_SUCCESS) {
        throw std::runtime_error("FrameRenderer::submitFrame() -> failed to submit draw command buffer");
    }
}

VkResult FrameRenderer::presentFrame(uint32_t p_image_index) {
    VkSemaphore signalSemaphores[] = {_renderer.getRenderFinishedSemaphore(p_image_index)};
    VkSwapchainKHR swapChains[] = {_swapchain.getSwapChain()};

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &p_image_index;

    return vkQueuePresentKHR(_device.getPresentQueue(), &presentInfo);
}

FrameRenderStatus FrameRenderer::render(const Camera& camera, const glm::vec4& p_clear_color) {
    syncCommandBufferRecordingState();

    vkWaitForFences(
        _device.getDevice(),
        1,
        &_renderer.getCurrentInFlightFences(),
        VK_TRUE,
        UINT64_MAX
    );

    uint32_t imageIndex = 0;
    const VkResult acquireResult = acquireFrameImage(imageIndex);
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        return FrameRenderStatus::NeedsRecreate;
    }

    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("FrameRenderer::render() -> failed to acquire swapchain image");
    }

    updateFrameResources(imageIndex, camera);
    ensureFrameCommandBufferRecorded(imageIndex, p_clear_color);
    submitFrame(imageIndex);
    _renderer.incrementeCurrentFrame();

    const VkResult presentResult = presentFrame(imageIndex);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
        return FrameRenderStatus::NeedsRecreate;
    }

    if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("FrameRenderer::render() -> failed to present swapchain image");
    }

    return FrameRenderStatus::Rendered;
}
