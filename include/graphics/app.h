#ifndef VULKAN_APP_HPP
#define VULKAN_APP_HPP

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <chrono>

#include "core/camera.h"
#include "core/config.h"
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
#include "graphics/vertex.h"
#include "graphics/block_update.h"

#include "world/chunk.h"
#include "world/voxel.h"

class VulkanApp {
public:
    void init(glm::vec3 posCamera, float fov);
    void render();
    void drawFrame();
    void recordCommandBuffer(uint32_t imageIndex);
    void cleanup();

    bool isRun() { return !glfwWindowShouldClose(window); };

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    GLFWwindow* getWindow() const { return window; }
    const float getDeltaTime() const { return deltaTime; }
    Camera* getCamera() { return &camera; }
    BufferManager& getBufferManager() { return bufferManager; }
    ChunkRenderStateCache& getChunkRenderStateCache() { return chunkRenderStateCache; }
    Renderer& getRenderer() { return renderer; }

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

    float deltaTime = 0;
    float lastFrame = 0;

    void initWindow();
    void initVulkan();

    void computeShader(std::vector<VkSemaphore>& waitSemaphores, std::vector<VkPipelineStageFlags>& waitStages);

    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);

    void updateDeltaTime();
};

#endif // VULKAN_APP_HPP
