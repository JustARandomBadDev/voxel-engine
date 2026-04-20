#include "graphics/swapchain.h"

#include <algorithm>
#include <limits>

#include "graphics/device.h"
#include "graphics/graphic_pipeline.h"
#include "graphics/instance.h"
#include "graphics/renderer.h"

void Swapchain::createSwapChain(GLFWwindow* window, Instance& p_instance, Device& p_device) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(p_device.getPhysicalDevice(), p_instance);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(window, swapChainSupport.capabilities);

    frames_in_flight = swapChainSupport.capabilities.minImageCount+1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && frames_in_flight > swapChainSupport.capabilities.maxImageCount) {
        frames_in_flight = swapChainSupport.capabilities.maxImageCount;
    }

    // std::cout << "Swapchain Capabilities : " << std::endl;
    // std::cout << "  Max : " << swapChainSupport.capabilities.minImageCount << std::endl;
    // std::cout << "  Min : " << swapChainSupport.capabilities.maxImageCount << std::endl;
    // std::cout << "  Current : " << frames_in_flight << std::endl;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = p_instance.getSurface();

    createInfo.minImageCount = frames_in_flight;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = p_device.findQueueFamilies(p_device.getPhysicalDevice(), p_instance);
    uint32_t queueFamilyIndices[] = {indices.graphicsAndComputeFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsAndComputeFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(p_device.getDevice(), &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(p_device.getDevice(), swapChain, &frames_in_flight, nullptr);
    swapChainImages.resize(frames_in_flight);
    vkGetSwapchainImagesKHR(p_device.getDevice(), swapChain, &frames_in_flight, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void Swapchain::recreateSwapChain(GLFWwindow* window, Instance& p_instance, GraphicPipeline& p_graphic_pipeline, Renderer& p_renderer, Device& p_device) {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(p_device.getDevice());

    cleanup(p_device);

    createSwapChain(window, p_instance, p_device);
    createImageViews(p_device);
    p_device.createDepthResources(*this);
    p_renderer.createFramebuffers(p_graphic_pipeline, *this, p_device);
}

void Swapchain::createImageViews(Device& p_device) {
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, p_device);
    }
}

void Swapchain::cleanup(Device& p_device) {
    vkDestroyImageView(p_device.getDevice(), p_device.getDepthImageView(), nullptr);
    vkDestroyImage(p_device.getDevice(), p_device.getDepthImage(), nullptr);
    vkFreeMemory(p_device.getDevice(), p_device.getDepthImageMemory(), nullptr);

    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(p_device.getDevice(), framebuffer, nullptr);
    }

    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(p_device.getDevice(), imageView, nullptr);
    }

    swapChainFramebuffers.clear();
    swapChainImageViews.clear();

    vkDestroySwapchainKHR(p_device.getDevice(), swapChain, nullptr);
}

VkImageView Swapchain::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, Device& p_device) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(p_device.getDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image view!");
        }

        return imageView;
    }

SwapChainSupportDetails Swapchain::querySwapChainSupport(VkPhysicalDevice pdevice, Instance& p_instance) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdevice, p_instance.getSurface(), &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(pdevice, p_instance.getSurface(), &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(pdevice, p_instance.getSurface(), &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(pdevice, p_instance.getSurface(), &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(pdevice, p_instance.getSurface(), &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::chooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void Swapchain::addSwapChainFramebuffers(VkFramebuffer pframeBuffer) {
    swapChainFramebuffers.push_back(pframeBuffer);
}
