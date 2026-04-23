#ifndef COMMAND_RECORDER_H
#define COMMAND_RECORDER_H

#include <cstdint>

#include <glm/vec4.hpp>
#include <vulkan/vulkan.h>

class BufferManager;
class Descriptor;
class GraphicPipeline;
class Renderer;
class Swapchain;

class CommandRecorder {
public:
    CommandRecorder(
        Renderer& p_renderer,
        Swapchain& p_swapchain,
        GraphicPipeline& p_graphic_pipeline,
        Descriptor& p_descriptor,
        BufferManager& p_buffer_manager
    );

    void record(uint32_t p_image_index, const glm::vec4& p_clear_color);

private:
    Renderer& _renderer;
    Swapchain& _swapchain;
    GraphicPipeline& _graphic_pipeline;
    Descriptor& _descriptor;
    BufferManager& _buffer_manager;

    void recordOpaqueDraws(VkCommandBuffer p_command_buffer, uint32_t p_image_index);
    void recordTransparentDraws(VkCommandBuffer p_command_buffer, uint32_t p_image_index);
};

#endif
