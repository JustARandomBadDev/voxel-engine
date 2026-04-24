#ifndef GRAPHICS_RUNTIME_LIFECYCLE_H
#define GRAPHICS_RUNTIME_LIFECYCLE_H

#include <vulkan/vulkan.h>

#include "engine/voxel_engine_config.h"

class BufferManager;
class Descriptor;
class Device;
class GraphicPipeline;
class Instance;
class Renderer;
class Swapchain;
class Texture;

// Coordinates graphics runtime initialization and swapchain-dependent recreate/cleanup.
// The subsystems it operates on are owned by GraphicsRuntime.
class GraphicsRuntimeLifecycle {
public:
    GraphicsRuntimeLifecycle(
        Instance& p_instance,
        Device& p_device,
        Renderer& p_renderer,
        Swapchain& p_swapchain,
        Texture& p_texture,
        Descriptor& p_descriptor,
        GraphicPipeline& p_graphic_pipeline,
        BufferManager& p_buffer_manager
    );

    void initialize(
        const VulkanHostConfig& p_host_config,
        const GraphicsResourceConfig& p_resources,
        const GpuAllocatorConfig& p_gpu_allocator_config,
        uint32_t p_frames_in_flight,
        bool p_enable_validation_layers,
        VkExtent2D p_framebuffer_extent
    );

    void recreateSwapchain(VkExtent2D p_framebuffer_extent);
    void cleanupSwapchainDependentResources();

private:
    Instance& _instance;
    Device& _device;
    Renderer& _renderer;
    Swapchain& _swapchain;
    Texture& _texture;
    Descriptor& _descriptor;
    GraphicPipeline& _graphic_pipeline;
    BufferManager& _buffer_manager;
    GraphicsResourceConfig _resources;
    uint32_t _frames_in_flight = 0;

    void createSwapchainResources(VkExtent2D p_framebuffer_extent, uint32_t p_frames_in_flight);
    void createSwapchainRenderTargets();
    void createPipelineResources();
    void createFrameResources(uint32_t p_frames_in_flight);
    void recreateFrameResources();
};

#endif
