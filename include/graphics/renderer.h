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
    void createCommandBuffers(Device& p_device, uint32_t p_image_count);
    void createSyncObjects(Device& p_device, uint32_t p_frames_in_flight, uint32_t p_image_count);
    void invalidateAllCommandBuffers();
    void resetCopyCommandBuffer();
    void cleanup(Device& p_device);

    const VkCommandPool& getCommandPool() const { return commandPool; }

    const VkCommandBuffer& getCommandBuffer(uint32_t imageIndex) const { return commandBuffers[imageIndex]; }

    bool isCommandBufferDirty(uint32_t imageIndex) const { return commandBufferDirty[imageIndex]; }
    void setCommandBufferDirty(uint32_t imageIndex, bool p_dirty) { commandBufferDirty[imageIndex] = p_dirty; }

    const std::vector<VkFence>& getInFlightFences() const { return inFlightFences; }

    uint32_t getCurrentFrame() const { return currentFrame; }
    uint32_t getFramesInFlight() const { return framesInFlight; }
    uint32_t getSwapchainImageCount() const { return swapchainImageCount; }
    const VkFence& getCurrentInFlightFences() const { return inFlightFences[currentFrame]; }

    const VkSemaphore& getCurrentImageAvailableSemaphores() const { return imageAvailableSemaphores[currentFrame]; }
    const VkSemaphore& getRenderFinishedSemaphore(uint32_t imageIndex) const { return renderFinishedSemaphores[imageIndex]; }

    const VkCommandBuffer& getCopyCommandBuffer() const { return copyCommandBuffer; }

    void incrementeCurrentFrame();

private:
    VkCommandPool commandPool = VK_NULL_HANDLE;
    
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<bool> commandBufferDirty;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    VkCommandBuffer copyCommandBuffer = VK_NULL_HANDLE;

    uint32_t framesInFlight = 0;
    uint32_t swapchainImageCount = 0;
    uint32_t currentFrame = 0;
};

#endif // VULKAN_RENDERER_HPP
