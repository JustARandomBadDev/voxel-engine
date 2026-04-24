#ifndef VULKAN_DEVICE_H
#define VULKAN_DEVICE_H

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsAndComputeFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const {
        return graphicsAndComputeFamily.has_value() && presentFamily.has_value();
    }
};

class Instance;
class Swapchain;

// Owns the selected physical/logical device, queues, and the current depth attachment resources.
class Device {
public:
    void pickPhysicalDevice(Instance& p_instance, Swapchain& p_swapchain);
    void createLogicalDevice(Instance& p_instance);
    void recreateDepthResources(Swapchain& p_swapchain);
    void cleanupDepthResources();
    void cleanup();

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice pdevice, Instance& p_instance) const;
    VkFormat findDepthFormat() const;

    VkPhysicalDevice& getPhysicalDevice() { return physicalDevice; }
    const VkPhysicalDevice& getPhysicalDevice() const { return physicalDevice; }
    VkDevice& getDevice() { return device; }
    const VkDevice& getDevice() const { return device; }

    VkQueue& getGraphicsQueue() { return graphicsQueue; }
    const VkQueue& getGraphicsQueue() const { return graphicsQueue; }
    VkQueue& getPresentQueue() { return presentQueue; }
    const VkQueue& getPresentQueue() const { return presentQueue; }
    
    VkImage& getDepthImage() { return depthImage; }
    const VkImage& getDepthImage() const { return depthImage; }
    VkDeviceMemory& getDepthImageMemory() { return depthImageMemory; }
    const VkDeviceMemory& getDepthImageMemory() const { return depthImageMemory; }
    VkImageView& getDepthImageView() { return depthImageView; }
    const VkImageView& getDepthImageView() const { return depthImageView; }

private:
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;

    bool isDeviceSuitable(VkPhysicalDevice pdevice, Instance& p_instance, Swapchain& p_swapchain) const;
    bool checkDeviceExtensionSupport(VkPhysicalDevice pdevice) const;
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
};

#endif // VULKAN_DEVICE_H
