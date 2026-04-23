#include "graphics/device.h"

#include <set>

#include "graphics/instance.h"
#include "graphics/swapchain.h"
#include "graphics/buffer_manager.h"

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Device selection requires queue-family support, swapchain support, required extensions, and multiDrawIndirect.
void Device::pickPhysicalDevice(Instance& p_instance, Swapchain& p_swapchain) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(p_instance.getInstance(), &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("Device::pickPhysicalDevice() -> no Vulkan-capable physical device found");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(p_instance.getInstance(), &deviceCount, devices.data());

    for (const auto& pdevice : devices) {
        if (isDeviceSuitable(pdevice, p_instance, p_swapchain)) {
            physicalDevice = pdevice;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Device::pickPhysicalDevice() -> failed to find a suitable physical device");
    }
}

// The runtime requires a graphics+compute-capable family and a present-capable family,
// creating one logical queue per unique required family.
void Device::createLogicalDevice(Instance& p_instance) {
    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Device::createLogicalDevice() -> physical device is not initialized");
    }

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, p_instance);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsAndComputeFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.multiDrawIndirect = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("Device::createLogicalDevice() -> failed to create logical device");
    }

    vkGetDeviceQueue(device, indices.graphicsAndComputeFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

// Depth image, memory, and view are device-owned but recreated for the current swapchain extent/render targets.
void Device::recreateDepthResources(Swapchain& p_swapchain) {
    VkFormat depthFormat = findDepthFormat();

    BufferManager::createImage(p_swapchain.getSwapChainExtent().width, p_swapchain.getSwapChainExtent().height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory, *this);
    depthImageView = p_swapchain.createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, *this);
}

// This releases only swapchain-dependent depth resources and may run across swapchain recreates.
void Device::cleanupDepthResources() {
    if (device == VK_NULL_HANDLE) return;

    if (depthImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, depthImageView, nullptr);
    }

    if (depthImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, depthImage, nullptr);
    }

    if (depthImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, depthImageMemory, nullptr);
    }

    depthImageView = VK_NULL_HANDLE;
    depthImage = VK_NULL_HANDLE;
    depthImageMemory = VK_NULL_HANDLE;
}

void Device::cleanup() {
    vkDestroyDevice(device, nullptr);
}

bool Device::isDeviceSuitable(VkPhysicalDevice pdevice, Instance& p_instance, Swapchain& p_swapchain) const {
    QueueFamilyIndices indices = findQueueFamilies(pdevice, p_instance);

    bool extensionsSupported = checkDeviceExtensionSupport(pdevice);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = p_swapchain.querySwapChainSupport(pdevice, p_instance);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(pdevice, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.multiDrawIndirect;
}

// The renderer expects one combined graphics+compute family plus a present-capable family.
QueueFamilyIndices Device::findQueueFamilies(VkPhysicalDevice pdevice, Instance& p_instance) const {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            indices.graphicsAndComputeFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(pdevice, i, p_instance.getSurface(), &presentSupport);

        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

bool Device::checkDeviceExtensionSupport(VkPhysicalDevice pdevice) const {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(pdevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(pdevice, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

VkFormat Device::findDepthFormat() const {
    return findSupportedFormat(
    {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkFormat Device::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("Device::findSupportedFormat() -> failed to find a supported depth format");
}
