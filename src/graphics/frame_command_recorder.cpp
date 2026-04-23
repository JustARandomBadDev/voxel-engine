#include "graphics/command_recorder.h"

#include <array>
#include <stdexcept>

#include "graphics/allocators_manager.h"
#include "graphics/buffer.h"
#include "graphics/buffer_manager.h"
#include "graphics/descriptor.h"
#include "graphics/graphic_pipeline.h"
#include "graphics/renderer.h"
#include "graphics/swapchain.h"

CommandRecorder::CommandRecorder(
    Renderer& p_renderer,
    Swapchain& p_swapchain,
    GraphicPipeline& p_graphic_pipeline,
    Descriptor& p_descriptor,
    BufferManager& p_buffer_manager
)
: _renderer(p_renderer),
  _swapchain(p_swapchain),
  _graphic_pipeline(p_graphic_pipeline),
  _descriptor(p_descriptor),
  _buffer_manager(p_buffer_manager) {}

void CommandRecorder::recordOpaqueDraws(VkCommandBuffer p_command_buffer, uint32_t p_image_index) {
    vkCmdBindPipeline(
        p_command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _graphic_pipeline.getOpaquePipeline()
    );
    vkCmdBindDescriptorSets(
        p_command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _graphic_pipeline.getOpaquePipelineLayout(),
        0,
        1,
        &_descriptor.getDescriptorSets()[p_image_index],
        0,
        nullptr
    );

    for (const AllocationPage& page : _buffer_manager.getAllocator().getPages()) {
        const uint32_t indirectCount = page.indirectAllocator.getCommittedBlockCount();
        if (indirectCount == 0) continue;

        const Buffer& vertexBuffer = _buffer_manager.getManagedBuffer(page.vertexAllocator.getBufferId());
        const Buffer& indexBuffer = _buffer_manager.getManagedBuffer(page.indexAllocator.getBufferId());
        const Buffer& indirectBuffer = _buffer_manager.getManagedBuffer(page.indirectAllocator.getBufferId());

        VkBuffer vertexBuffers[] = {vertexBuffer.getBuffer()};
        VkDeviceSize offsets[] = {0};

        vkCmdBindVertexBuffers(p_command_buffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(p_command_buffer, indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexedIndirect(
            p_command_buffer,
            indirectBuffer.getBuffer(),
            0,
            indirectCount,
            sizeof(DrawIndirectCommand)
        );
    }
}

void CommandRecorder::recordTransparentDraws(VkCommandBuffer p_command_buffer, uint32_t p_image_index) {
    vkCmdBindPipeline(
        p_command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _graphic_pipeline.getTransparentPipeline()
    );
    vkCmdBindDescriptorSets(
        p_command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _graphic_pipeline.getTransparentPipelineLayout(),
        0,
        1,
        &_descriptor.getDescriptorSets()[p_image_index],
        0,
        nullptr
    );

    for (const AllocationPage& page : _buffer_manager.getTransparentAllocator().getPages()) {
        const uint32_t indirectCount = page.indirectAllocator.getCommittedBlockCount();
        if (indirectCount == 0) continue;

        const Buffer& vertexBuffer = _buffer_manager.getManagedBuffer(page.vertexAllocator.getBufferId());
        const Buffer& indexBuffer = _buffer_manager.getManagedBuffer(page.indexAllocator.getBufferId());
        const Buffer& indirectBuffer = _buffer_manager.getManagedBuffer(page.indirectAllocator.getBufferId());

        VkDeviceSize offset = 0;
        VkBuffer vertexBufferHandle = vertexBuffer.getBuffer();

        vkCmdBindVertexBuffers(
            p_command_buffer,
            0,
            1,
            &vertexBufferHandle,
            &offset
        );
        vkCmdBindIndexBuffer(p_command_buffer, indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexedIndirect(
            p_command_buffer,
            indirectBuffer.getBuffer(),
            0,
            indirectCount,
            sizeof(DrawIndirectCommand)
        );
    }
}

void CommandRecorder::record(uint32_t p_image_index, const glm::vec4& p_clear_color) {
    const VkCommandBuffer command = _renderer.getCommandBuffer(p_image_index);

    vkResetCommandBuffer(command, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(command, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("FrameCommandRecorder::record() -> failed to begin command buffer recording");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = _graphic_pipeline.getRenderPass();
    renderPassInfo.framebuffer = _swapchain.getSwapChainFramebuffers()[p_image_index];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = _swapchain.getSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{
        p_clear_color.r,
        p_clear_color.g,
        p_clear_color.b,
        p_clear_color.a
    }};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(command, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_swapchain.getSwapChainExtent().width);
    viewport.height = static_cast<float>(_swapchain.getSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = _swapchain.getSwapChainExtent();
    vkCmdSetScissor(command, 0, 1, &scissor);

    recordOpaqueDraws(command, p_image_index);
    recordTransparentDraws(command, p_image_index);

    vkCmdEndRenderPass(command);

    if (vkEndCommandBuffer(command) != VK_SUCCESS) {
        throw std::runtime_error("FrameCommandRecorder::record() -> failed to record command buffer");
    }
}
