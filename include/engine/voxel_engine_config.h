#ifndef VOXEL_ENGINE_CONFIG_H
#define VOXEL_ENGINE_CONFIG_H

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

struct VulkanHostConfig {
    std::vector<std::string> requiredInstanceExtensions;
    std::function<VkResult(VkInstance, VkSurfaceKHR&)> createSurface;
    std::function<VkExtent2D()> getFramebufferExtent;
};

struct GraphicsResourceConfig {
    std::filesystem::path terrainTexture;
    std::filesystem::path voxelVertexShader;
    std::filesystem::path voxelFragmentShader;
};

struct GpuAllocatorConfig {
    uint32_t meshDataBlockCapacityPerAllocator = 1'048'576;
    uint32_t indirectCommandCapacityPerAllocator = 4'096;
    uint32_t stagingBufferBytes = 4 * 1024 * 1024;
    uint32_t allocationMarginBlocks = 1;
};

struct VoxelEngineInitConfig {
    VulkanHostConfig vulkanHost;
    glm::vec4 clearColor = {0.f, 0.f, 1.f, 1.0f};
    uint32_t framesInFlight = 2;
    bool enableValidationLayers = true;
    GraphicsResourceConfig graphicsResources;
    GpuAllocatorConfig gpuAllocator;
};

#endif
