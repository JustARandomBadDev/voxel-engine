#ifndef VOXEL_ENGINE_CONFIG_H
#define VOXEL_ENGINE_CONFIG_H

#include <filesystem>

#include <glm/glm.hpp>

struct GraphicsResourceConfig {
    std::filesystem::path terrainTexture;
    std::filesystem::path voxelVertexShader;
    std::filesystem::path voxelFragmentShader;
    std::filesystem::path meshingComputeShader;
};

struct VoxelEngineInitConfig {
    glm::vec3 cameraPos;
    float fov = 0.0f;
    GraphicsResourceConfig graphicsResources;
};

#endif
