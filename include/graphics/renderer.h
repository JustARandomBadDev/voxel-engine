#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

#include <vulkan/vulkan.h>
#include <vector>

class Device;
class GraphicPipeline;
class Instance;
class Swapchain;

class Renderer {
public:
    void createCommandPool(Device& p_device, Instance& p_instance);
    void createCommandBuffers(Device& p_device, uint32_t p_frames_in_flight);
    void createComputeCommandPool(uint32_t computeQueueFamily);
    void createComputeCommandBuffers(Device& p_device, uint32_t p_frames_in_flight);
    void createSyncObjects(Device& p_device, uint32_t p_frames_in_flight);
    void createFramebuffers(GraphicPipeline& p_graphic_pipeline, Swapchain& p_swapchain, Device& p_device);
    void resetCommandBuffers();
    void resetCopyCommandBuffer();
    void cleanup(Device& p_device);

    const VkCommandPool& getCommandPool() { return commandPool; }

    const VkCommandBuffer& getCurrentCommandBuffers() { return commandBuffers[currentFrame]; }
    const VkCommandBuffer& getCommandBuffer(int index) { return commandBuffers[index]; }

    const bool getCurrentCommandBuffersState() const { return commandBufferState[currentFrame]; }
    const void setCurrentCommandBuffersState(bool p_state) { commandBufferState[currentFrame] = p_state; }

    const VkCommandBuffer& getCurrentComputeCommandBuffers() { return computeCommandBuffers[currentFrame]; }
    const std::vector<VkFence>& getInFlightFences() { return inFlightFences; }
    const std::vector<VkFence>& getComputeInFlightFences() { return computeInFlightFences; }

    const uint32_t& getCurrentFrame() { return currentFrame; }
    const VkFence& getCurrentInFlightFences() { return inFlightFences[currentFrame]; }
    const VkFence& getCurrentComputeInFlightFences() { return computeInFlightFences[currentFrame]; }

    const VkSemaphore& getCurrentImageAvailableSemaphores() { return imageAvailableSemaphores[currentFrame]; }
    const VkSemaphore& getRenderFinishedSemaphore(uint32_t imageIndex) { return renderFinishedSemaphores[imageIndex]; }
    const VkSemaphore& getCurrentComputeFinishedSemaphores() { return computeFinishedSemaphores[currentFrame]; }

    const VkCommandBuffer& getCopyCommandBuffer() { return copyCommandBuffer; };

    void incrementeCurrentFrame();

private:
    VkCommandPool commandPool;
    
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<bool> commandBufferState;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    std::vector<VkCommandBuffer> computeCommandBuffers;

    std::vector<VkSemaphore> computeFinishedSemaphores;
    std::vector<VkFence> computeInFlightFences;

    VkCommandBuffer copyCommandBuffer;

    uint32_t framesInFlight = 0;
    uint32_t currentFrame = 0;
};

#endif // VULKAN_RENDERER_HPP
