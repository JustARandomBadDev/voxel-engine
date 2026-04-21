#ifndef VULKAN_APP_HPP
#define VULKAN_APP_HPP

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "core/camera.h"
#include "engine/voxel_engine_config.h"
#include "graphics/buffer.h"
#include "graphics/buffer_manager.h"
#include "graphics/chunk_render_state.h"
#include "graphics/compute_pipeline.h"
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
    void render();
    void drawFrame();
    void recordCommandBuffer(uint32_t imageIndex);
    void cleanup();

    bool isRun() const { return !glfwWindowShouldClose(window); };

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    GLFWwindow*                  getWindow()                      { return window; }
    const GLFWwindow*            getWindow()                const { return window; }
    float                        getDeltaTime()             const { return deltaTime; }
    Camera*                      getCamera()                      { return &camera; }
    const Camera*                getCamera()                const { return &camera; }
    BufferManager&               getBufferManager()               { return bufferManager; }
    const BufferManager&         getBufferManager()         const { return bufferManager; }
    ChunkRenderStateCache&       getChunkRenderStateCache()       { return chunkRenderStateCache; }
    const ChunkRenderStateCache& getChunkRenderStateCache() const { return chunkRenderStateCache; }
    Renderer&                    getRenderer()                    { return renderer; }
    const Renderer&              getRenderer()              const { return renderer; }

    VulkanApp()
    : camera({0, 0, 0}, 0, 0) {};

private:
    GLFWwindow* window;

    Instance instance;
    Device device;
    BufferManager bufferManager;
    ChunkRenderStateCache chunkRenderStateCache;
    Renderer renderer;
    Swapchain swapchain;
    Texture texture;
    Descriptor descriptor;
    GraphicPipeline graphicPipeline;
    ComputePipeline computePipeline;

    Camera camera;

    bool framebufferResized = false;
    int generated;
    uint32_t _last_opaque_indirect_count = 0;
    uint32_t _last_transparent_indirect_count = 0;

    float deltaTime = 0;
    float lastFrame = 0;

    void initWindow();
    void initVulkan(const GraphicsResourceConfig& resources, const GpuAllocatorConfig& gpu_allocator_config, uint32_t p_frames_in_flight);
    void recreateSwapchainResources();

    void computeShader(std::vector<VkSemaphore>& waitSemaphores, std::vector<VkPipelineStageFlags>& waitStages);

    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);

    void updateDeltaTime();
};

#endif // VULKAN_APP_HPP
