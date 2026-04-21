#include "world/chunk.h"

Chunk::Chunk(glm::ivec3 p_pos)
: _pos(p_pos) {}

void Chunk::addVoxel(glm::ivec3 p_pos, Voxel p_voxel) {
    _voxels[p_pos.x][p_pos.y][p_pos.z] = p_voxel;
    _is_modify = true;
}
