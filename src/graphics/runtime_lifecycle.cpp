#include "graphics/runtime_lifecycle.h"

#include <stdexcept>

#include "graphics/buffer_manager.h"
#include "graphics/descriptor.h"
#include "graphics/device.h"
#include "graphics/graphic_pipeline.h"
#include "graphics/instance.h"
#include "graphics/renderer.h"
#include "graphics/swapchain.h"
#include "graphics/texture.h"

GraphicsRuntimeLifecycle::GraphicsRuntimeLifecycle(
    Instance& p_instance,
    Device& p_device,
    Renderer& p_renderer,
    Swapchain& p_swapchain,
    Texture& p_texture,
    Descriptor& p_descriptor,
    GraphicPipeline& p_graphic_pipeline,
    BufferManager& p_buffer_manager
)
: _instance(p_instance),
  _device(p_device),
  _renderer(p_renderer),
  _swapchain(p_swapchain),
  _texture(p_texture),
  _descriptor(p_descriptor),
  _graphic_pipeline(p_graphic_pipeline),
  _buffer_manager(p_buffer_manager) {}

void GraphicsRuntimeLifecycle::createSwapchainResources(VkExtent2D p_framebuffer_extent, uint32_t p_frames_in_flight) {
    if (p_framebuffer_extent.width == 0 || p_framebuffer_extent.height == 0) {
        throw std::runtime_error("GraphicsRuntimeLifecycle::createSwapchainResources() -> host framebuffer extent must be non-zero");
    }

    _swapchain.createSwapChain(p_framebuffer_extent, _instance, _device, p_frames_in_flight);
    _swapchain.createImageViews(_device);
}

void GraphicsRuntimeLifecycle::createPipelineResources() {
    _graphic_pipeline.createRenderPass(_swapchain, _device);
    _graphic_pipeline.createGraphicsPipeline(
        _resources.voxelVertexShader,
        _resources.voxelFragmentShader,
        _device
    );
}

void GraphicsRuntimeLifecycle::createSwapchainRenderTargets() {
    _device.recreateDepthResources(_swapchain);
    _swapchain.createFramebuffers(_graphic_pipeline, _device);
}

void GraphicsRuntimeLifecycle::createFrameResources(uint32_t p_frames_in_flight) {
    _buffer_manager.createUniformBuffers(_swapchain.getImageCount());

    _descriptor.createDescriptorPool(_device, _swapchain.getImageCount());
    _descriptor.createDescriptorSets(
        _buffer_manager,
        _texture,
        _graphic_pipeline,
        _device,
        _swapchain.getImageCount()
    );

    _renderer.createCommandBuffers(_device, _swapchain.getImageCount());
    _renderer.createSyncObjects(_device, p_frames_in_flight, _swapchain.getImageCount());
}

void GraphicsRuntimeLifecycle::recreateFrameResources() {
    _buffer_manager.cleanupUniformBuffer();
    _descriptor.cleanup(_device);
    createFrameResources(_renderer.getFramesInFlight());
    _renderer.invalidateAllCommandBuffers();
}

void GraphicsRuntimeLifecycle::cleanupSwapchainDependentResources() {
    _swapchain.cleanupFramebuffers(_device);
    _device.cleanupDepthResources();
    _buffer_manager.cleanupUniformBuffer();
    _descriptor.cleanup(_device);
    _graphic_pipeline.cleanup(_device);
}

void GraphicsRuntimeLifecycle::initialize(
    const VulkanHostConfig& p_host_config,
    const GraphicsResourceConfig& p_resources,
    const GpuAllocatorConfig& p_gpu_allocator_config,
    uint32_t p_frames_in_flight,
    bool p_enable_validation_layers,
    VkExtent2D p_framebuffer_extent
) {
    _resources = p_resources;
    _frames_in_flight = p_frames_in_flight;

    _instance.createInstance(p_enable_validation_layers, p_host_config.requiredInstanceExtensions);
    _instance.setupDebugMessenger();
    _instance.createSurface(p_host_config.createSurface);

    _device.pickPhysicalDevice(_instance, _swapchain);
    _device.createLogicalDevice(_instance);

    _buffer_manager.configure(_device, _renderer);

    createSwapchainResources(p_framebuffer_extent, p_frames_in_flight);

    _graphic_pipeline.createDescriptorSetLayout(_device);
    createPipelineResources();

    _renderer.createCommandPool(_device, _instance);
    createSwapchainRenderTargets();

    _texture.createTextureImage(p_resources.terrainTexture, _renderer, _device);
    _texture.createTextureImageView(_swapchain, _device);
    _texture.createTextureSampler(_device);

    _buffer_manager.createBuffers(p_gpu_allocator_config);
    createFrameResources(p_frames_in_flight);
}

void GraphicsRuntimeLifecycle::recreateSwapchain(VkExtent2D p_framebuffer_extent) {
    vkDeviceWaitIdle(_device.getDevice());

    // 1. Destroy resources that are tied to the current swapchain images/format.
    cleanupSwapchainDependentResources();
    _swapchain.cleanup(_device);

    // 2. Rebuild swapchain storage and its dependent render targets in a fixed order.
    createSwapchainResources(p_framebuffer_extent, _frames_in_flight);
    createPipelineResources();
    createSwapchainRenderTargets();
    recreateFrameResources();
}
