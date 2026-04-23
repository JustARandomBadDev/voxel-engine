#ifndef FRAME_RENDERER_H
#define FRAME_RENDERER_H

#include <cstdint>

#include <glm/vec4.hpp>
#include <vulkan/vulkan.h>

class BufferManager;
class Camera;
class Device;
class CommandRecorder;
class Renderer;
class Swapchain;

enum class FrameRenderStatus {
    Rendered,
    NeedsRecreate
};

class FrameRenderer {
public:
    FrameRenderer(
        Device& p_device,
        Renderer& p_renderer,
        Swapchain& p_swapchain,
        BufferManager& p_buffer_manager,
        CommandRecorder& p_command_recorder
    );

    FrameRenderStatus render(const Camera& camera, const glm::vec4& p_clear_color);
    void onFrameResourcesRecreated();

private:
    Device& _device;
    Renderer& _renderer;
    Swapchain& _swapchain;
    BufferManager& _buffer_manager;
    CommandRecorder& _command_recorder;

    uint32_t _last_opaque_indirect_count = 0;
    uint32_t _last_transparent_indirect_count = 0;

    void refreshIndirectCounts();
    void syncCommandBufferRecordingState();
    VkResult acquireFrameImage(uint32_t& p_image_index);
    void updateFrameResources(uint32_t p_image_index, const Camera& camera);
    void ensureFrameCommandBufferRecorded(uint32_t p_image_index, const glm::vec4& p_clear_color);
    void submitFrame(uint32_t p_image_index);
    VkResult presentFrame(uint32_t p_image_index);
};

#endif
