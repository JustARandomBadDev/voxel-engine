#ifndef CHUNK_H
#define CHUNK_H

#include <glm/glm.hpp>

#include "core/config.h"
#include "world/voxel.h"

class Chunk {
public:
    Chunk() = default;
    explicit Chunk(glm::ivec3 p_pos);
    ~Chunk() = default;

    void addVoxel(glm::ivec3 p_pos, Voxel p_voxel);
    void removeVoxel(glm::ivec3 p_pos) { addVoxel(p_pos, Voxel(0)); };

    glm::ivec3 getPos() const { return _pos; }

    Voxel      getVoxel(int x, int y, int z) { return _voxels[x][y][z]; }
    Voxel      getVoxel(int x, int y, int z) const { return _voxels[x][y][z]; }

    // This state means the chunk still needs to pass through CPU meshing and render synchronization.
    // It is intentionally broader than a simple local "modified" flag.
    bool       needsRenderSync() const { return _is_modify; }
    void       markRenderSyncDirty() { _is_modify = true; }
    void       clearRenderSyncDirty() { _is_modify = false; }
    
    // Raw voxel storage indexed by chunk-local coordinates in [0, CHUNK_SIZE) on each axis.
    Voxel (&getVoxels()) [CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE] { return _voxels; }
    const Voxel (&getVoxels() const) [CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE] { return _voxels; }
    
    Voxel getVoxel(glm::vec3 p_pos) {
        return getVoxel(
            static_cast<int>(p_pos.x),
            static_cast<int>(p_pos.y),
            static_cast<int>(p_pos.z)
        );
    }

    Voxel getVoxel(glm::vec3 p_pos) const {
        return getVoxel(
            static_cast<int>(p_pos.x),
            static_cast<int>(p_pos.y),
            static_cast<int>(p_pos.z)
        );
    }

private:
    glm::ivec3 _pos = {0, 0, 0};
    Voxel _voxels[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE] = {};
    bool _is_modify = false;
};

#endif
