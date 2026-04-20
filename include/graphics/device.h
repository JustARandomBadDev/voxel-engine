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

class Device {
public:
    void pickPhysicalDevice(Instance& p_instance, Swapchain& p_swapchain);
    void createLogicalDevice(Instance& p_instance);
    void createDepthResources(Swapchain& p_swapchain);
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
    VkQueue& getComputeQueue() { return computeQueue; }
    const VkQueue& getComputeQueue() const { return computeQueue; }
    
    VkImage& getDepthImage() { return depthImage; }
    const VkImage& getDepthImage() const { return depthImage; }
    VkDeviceMemory& getDepthImageMemory() { return depthImageMemory; }
    const VkDeviceMemory& getDepthImageMemory() const { return depthImageMemory; }
    VkImageView& getDepthImageView() { return depthImageView; }
    const VkImageView& getDepthImageView() const { return depthImageView; }

private:
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue computeQueue;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    bool isDeviceSuitable(VkPhysicalDevice pdevice, Instance& p_instance, Swapchain& p_swapchain) const;
    bool checkDeviceExtensionSupport(VkPhysicalDevice pdevice) const;
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
};

#endif // VULKAN_DEVICE_H
