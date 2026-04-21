#ifndef VOXEL_ENGINE_CONFIG_H
#define VOXEL_ENGINE_CONFIG_H

#include <cstdint>
#include <filesystem>

#include <glm/glm.hpp>

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
    float fov = 0.0f;
    uint32_t framesInFlight = 2;
    GraphicsResourceConfig graphicsResources;
    GpuAllocatorConfig gpuAllocator;
};

#endif
