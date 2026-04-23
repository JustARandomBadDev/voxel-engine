#ifndef VULKAN_APP_HPP
#define VULKAN_APP_HPP

#include <vulkan/vulkan.h>

#include "core/camera.h"
#include "engine/voxel_engine_config.h"
#include "graphics/buffer_manager.h"
#include "graphics/chunk_render_state.h"
#include "graphics/descriptor.h"
#include "graphics/device.h"
#include "graphics/command_recorder.h"
#include "graphics/frame_renderer.h"
#include "graphics/instance.h"
#include "graphics/graphic_pipeline.h"
#include "graphics/renderer.h"
#include "graphics/runtime_lifecycle.h"
#include "graphics/swapchain.h"
#include "graphics/texture.h"

class GraphicsRuntime {
public:
    GraphicsRuntime();

    void init(const VoxelEngineInitConfig& config);
    void render(const Camera& camera);
    void cleanup();

    BufferManager& getBufferManager() { return bufferManager; }
    const BufferManager& getBufferManager() const { return bufferManager; }

    ChunkRenderStateCache& getChunkRenderStateCache() { return chunkRenderStateCache; }
    const ChunkRenderStateCache& getChunkRenderStateCache() const { return chunkRenderStateCache; }

    float getAspectRatio() const;

private:
    Instance instance;
    Device device;
    BufferManager bufferManager;
    ChunkRenderStateCache chunkRenderStateCache;
    Renderer renderer;
    Swapchain swapchain;
    Texture texture;
    Descriptor descriptor;
    GraphicPipeline graphicPipeline;
    GraphicsRuntimeLifecycle runtimeLifecycle;
    CommandRecorder frameCommandRecorder;
    FrameRenderer frameRenderer;

    glm::vec4 _clear_color = {0.f, 0.f, 1.f, 1.0f};
    VulkanHostConfig _host_config;
    bool _swapchain_needs_recreate = false;

    bool recreateSwapchain();
    VkExtent2D getFramebufferExtent() const;
    bool ensureSwapchainReady();
};

#endif // VULKAN_APP_HPP
