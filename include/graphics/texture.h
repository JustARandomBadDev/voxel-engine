#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <filesystem>
#include <vulkan/vulkan.h>

class Device;
class Renderer;
class Swapchain;

class Texture {
public:
    void createTextureImage(
        const std::filesystem::path& texture_path,
        Renderer& p_renderer,
        Device& p_device
    );
    void createTextureImageView(Swapchain& p_swapchain, Device& p_device);
    void createTextureSampler(Device& p_device);
    void cleanup(Device& p_device);

    const VkImageView& getTextureImageView() const { return textureImageView; }
    const VkSampler& getTextureSampler() const { return textureSampler; }

private:
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, Renderer& p_renderer, Device& p_device);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, Renderer& p_renderer, Device& p_device);
};

#endif // TEXTURE_MANAGER_HPP
