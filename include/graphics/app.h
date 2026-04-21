#ifndef VULKAN_APP_HPP
#define VULKAN_APP_HPP

#include <vulkan/vulkan.h>

#include "core/camera.h"
#include "engine/voxel_engine_config.h"
#include "graphics/buffer.h"
#include "graphics/buffer_manager.h"
#include "graphics/chunk_render_state.h"
#include "graphics/descriptor.h"
#include "graphics/device.h"
#include "graphics/instance.h"
#include "graphics/graphic_pipeline.h"
#include "graphics/renderer.h"
#include "graphics/swapchain.h"
#include "graphics/texture.h"

class VulkanApp {
public:
    void init(const VoxelEngineInitConfig& config);
    void render(const Camera& camera);
    void drawFrame(const Camera& camera);
    void recordCommandBuffer(uint32_t imageIndex);
    void cleanup();

    BufferManager&               getBufferManager()               { return bufferManager; }
    const BufferManager&         getBufferManager()         const { return bufferManager; }
    ChunkRenderStateCache&       getChunkRenderStateCache()       { return chunkRenderStateCache; }
    const ChunkRenderStateCache& getChunkRenderStateCache() const { return chunkRenderStateCache; }
    Renderer&                    getRenderer()                    { return renderer; }
    const Renderer&              getRenderer()              const { return renderer; }
    float                        getAspectRatio()           const;

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

    uint32_t _last_opaque_indirect_count = 0;
    uint32_t _last_transparent_indirect_count = 0;
    glm::vec4 _clear_color = {0.f, 0.f, 1.f, 1.0f};
    VulkanHostConfig _host_config;
    bool _swapchain_needs_recreate = false;

    void initVulkan(const GraphicsResourceConfig& resources, const GpuAllocatorConfig& gpu_allocator_config, uint32_t p_frames_in_flight, bool p_enable_validation_layers);
    bool recreateSwapchainResources();
    VkExtent2D getFramebufferExtent() const;
    bool syncSwapchainToHostExtent();
};

#endif // VULKAN_APP_HPP
