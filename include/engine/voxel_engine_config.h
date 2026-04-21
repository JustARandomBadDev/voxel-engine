#ifndef VOXEL_ENGINE_CONFIG_H
#define VOXEL_ENGINE_CONFIG_H

#include <cstdint>
#include <filesystem>
#include <string>

#include <glm/glm.hpp>

enum class WindowCursorMode {
    Captured,
    Normal
};

struct GraphicsResourceConfig {
    std::filesystem::path terrainTexture;
    std::filesystem::path voxelVertexShader;
    std::filesystem::path voxelFragmentShader;
    std::filesystem::path meshingComputeShader;
};

struct GpuAllocatorConfig {
    uint32_t meshDataBlockCapacityPerAllocator = 1'048'576;
    uint32_t indirectCommandCapacityPerAllocator = 4'096;
    uint32_t stagingBufferBytes = 4 * 1024 * 1024;
    uint32_t allocationMarginBlocks = 1;
};

struct VoxelEngineInitConfig {
    glm::vec3 cameraPos;
    uint32_t windowWidth = 1280;
    uint32_t windowHeight = 720;
    std::string windowTitle = "Voxel Sandbox";
    WindowCursorMode cursorMode = WindowCursorMode::Captured;

    float fov = 70.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float cameraSpeed = 10.0f;
    float mouseSensitivity = 0.1f;

    glm::vec4 clearColor = {0.f, 0.f, 1.f, 1.0f};
    uint32_t framesInFlight = 2;
    bool enableValidationLayers = true;
    GraphicsResourceConfig graphicsResources;
    GpuAllocatorConfig gpuAllocator;
};

#endif
