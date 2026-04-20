#ifndef VULKAN_SWAPCHAIN_H
#define VULKAN_SWAPCHAIN_H

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Device;
class GraphicPipeline;
class Instance;
class Renderer;

class Swapchain {
public:
    void createSwapChain(GLFWwindow* window, Instance& p_instance, Device& p_device);
    void recreateSwapChain(GLFWwindow* window, Instance& p_instance, GraphicPipeline& p_graphic_pipeline, Renderer& p_renderer, Device& p_device);
    void createImageViews(Device& p_device);
    void cleanup(Device& p_device);

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice pdevice, Instance& p_instance) const;
    VkImageView             createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, Device& p_device) const;

    const VkSwapchainKHR&             getSwapChain() const { return swapChain; }
    const VkFormat&                   getSwapChainImageFormat() const { return swapChainImageFormat; }
    const VkExtent2D&                 getSwapChainExtent() const { return swapChainExtent; }
    const std::vector<VkImageView>&   getSwapChainImageViews() const { return swapChainImageViews; }
    const std::vector<VkFramebuffer>& getSwapChainFramebuffers() const { return swapChainFramebuffers; }
    const VkFramebuffer&              getFramebuffers(uint32_t imageIndex) const { return swapChainFramebuffers[imageIndex]; }

    float    getAspectRatio() const { return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height); }
    uint32_t getFramesInFlight() const { return frames_in_flight; }

    void addSwapChainFramebuffers(VkFramebuffer pframeBuffer);

private:
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    uint32_t frames_in_flight;

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities);

};

#endif // VULKAN_SWAPCHAIN_HPP
