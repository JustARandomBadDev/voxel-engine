#ifndef CHUNK_H
#define CHUNK_H

#include <glm/glm.hpp>

#include "core/config.h"
#include "world/voxel.h"

class Chunk {
public:
    Chunk();
    Chunk(glm::ivec3 p_pos);
    ~Chunk();

    void init();
    void addVoxel(glm::ivec3 p_pos, Voxel p_voxel);
    void removeVoxel(glm::ivec3 p_pos) { addVoxel(p_pos, Voxel(0)); };

    glm::ivec3 getPos()                      { return _pos; }
    Voxel      getVoxel(int x, int y, int z) { return _voxels[x][y][z]; }
    Voxel      getVoxel(int x, int y, int z) const { return _voxels[x][y][z]; }
    bool       isDirty() const               { return _is_modify; }
    void       clearDirty()                  { _is_modify = false; }
    Voxel (&getVoxels()) [CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE] { return _voxels; }
    
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
    glm::ivec3 _pos;

    Voxel _voxels[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];

    bool _is_modify;
};

#endif
