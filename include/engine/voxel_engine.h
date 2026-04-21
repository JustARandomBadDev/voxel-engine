#ifndef VOXEL_ENGINE_H
#define VOXEL_ENGINE_H

#include <memory>

#include <glm/glm.hpp>

#include "core/config.h"
#include "engine/voxel_engine_config.h"
#include "world/voxel.h"

class VoxelEngine {
public:
    VoxelEngine();
    ~VoxelEngine();

    VoxelEngine(const VoxelEngine&) = delete;
    VoxelEngine& operator=(const VoxelEngine&) = delete;
    VoxelEngine(VoxelEngine&&) noexcept;
    VoxelEngine& operator=(VoxelEngine&&) noexcept;

    void init(const VoxelEngineInitConfig& config);
    void update();
    void render();
    void shutdown();
    bool isRun() const;
    void createChunk(glm::ivec3 pos);
    void removeChunk(glm::ivec3 pos);
    void setVoxel(glm::ivec3 chunk_pos, glm::ivec3 local_voxel_pos, Voxel voxel);
    void removeVoxel(glm::ivec3 chunk_pos, glm::ivec3 local_voxel_pos);
    Voxel getVoxel(glm::ivec3 chunk_pos, glm::ivec3 local_voxel_pos) const;

private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};

#endif
