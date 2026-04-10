#include "world/chunk.h"

Chunk::Chunk()
: _is_modify(false) {}

Chunk::Chunk(glm::ivec3 p_pos)
: _pos(p_pos), _is_modify(false) {}

Chunk::~Chunk() = default;

void Chunk::init() {
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                _voxels[x][y][z] = Voxel(0);
            }
        }
    }
}

void Chunk::addVoxel(glm::ivec3 p_pos, Voxel p_voxel) {
    _voxels[p_pos.x][p_pos.y][p_pos.z] = p_voxel;
    _is_modify = true;
}
