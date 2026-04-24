#include "graphics/swapchain.h"

#include <array>
#include <algorithm>
#include <limits>

#include "graphics/device.h"
#include "graphics/graphic_pipeline.h"
#include "graphics/instance.h"

namespace {
constexpr VkPresentModeKHR kPreferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
constexpr VkPresentModeKHR kFallbackPresentMode = VK_PRESENT_MODE_FIFO_KHR;
}

// Request enough images to sustain the configured frames-in-flight while respecting surface limits.
void Swapchain::createSwapChain(VkExtent2D p_framebuffer_extent, Instance& p_instance, Device& p_device, uint32_t p_frames_in_flight) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(p_device.getPhysicalDevice(), p_instance);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(p_framebuffer_extent, swapChainSupport.capabilities);

    uint32_t requestedImageCount = std::max(
        p_frames_in_flight,
        swapChainSupport.capabilities.minImageCount + 1
    );

    if (swapChainSupport.capabilities.maxImageCount > 0) {
        if (p_frames_in_flight > swapChainSupport.capabilities.maxImageCount) {
            throw std::runtime_error("Swapchain::createSwapChain() -> configured framesInFlight exceeds supported swapchain image count");
        }

        requestedImageCount = std::min(requestedImageCount, swapChainSupport.capabilities.maxImageCount);
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = p_instance.getSurface();

    createInfo.minImageCount = requestedImageCount;
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
        throw std::runtime_error("Swapchain::createSwapChain() -> failed to create swapchain");
    }

    vkGetSwapchainImagesKHR(p_device.getDevice(), swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(p_device.getDevice(), swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void Swapchain::createImageViews(Device& p_device) {
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, p_device);
    }
}

// Framebuffers combine the current swapchain image views with the current depth attachment, so they are swapchain-dependent.
void Swapchain::createFramebuffers(GraphicPipeline& p_graphic_pipeline, Device& p_device) {
    swapChainFramebuffers.clear();
    swapChainFramebuffers.reserve(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            swapChainImageViews[i],
            p_device.getDepthImageView()
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = p_graphic_pipeline.getRenderPass();
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        if (vkCreateFramebuffer(p_device.getDevice(), &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
            throw std::runtime_error("Swapchain::createFramebuffers() -> failed to create framebuffer");
        }

        swapChainFramebuffers.push_back(framebuffer);
    }
}

// Reusable framebuffer-only cleanup used during swapchain-dependent teardown.
void Swapchain::cleanupFramebuffers(Device& p_device) {
    for (VkFramebuffer framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(p_device.getDevice(), framebuffer, nullptr);
    }

    swapChainFramebuffers.clear();
}

// Destroy swapchain-owned child resources before releasing the swapchain handle itself.
void Swapchain::cleanup(Device& p_device) {
    cleanupFramebuffers(p_device);

    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(p_device.getDevice(), imageView, nullptr);
    }

    swapChainFramebuffers.clear();
    swapChainImages.clear();
    swapChainImageViews.clear();
    imageCount = 0;

    vkDestroySwapchainKHR(p_device.getDevice(), swapChain, nullptr);
    swapChain = VK_NULL_HANDLE;
}

VkImageView Swapchain::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, Device& p_device) const {
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
            throw std::runtime_error("Swapchain::createImageView() -> failed to create swapchain image view");
        }

        return imageView;
    }

SwapChainSupportDetails Swapchain::querySwapChainSupport(VkPhysicalDevice pdevice, Instance& p_instance) const {
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
        if (availablePresentMode == kPreferredPresentMode) {
            return availablePresentMode;
        }
    }

    return kFallbackPresentMode;
}

// Some platforms provide a fixed surface extent; others require clamping the host framebuffer size to supported limits.
VkExtent2D Swapchain::chooseSwapExtent(VkExtent2D p_framebuffer_extent, const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = p_framebuffer_extent;

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}
