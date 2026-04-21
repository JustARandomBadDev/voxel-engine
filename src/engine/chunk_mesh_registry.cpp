#include "engine/chunk_mesh_registry.h"

MeshBuilder& ChunkMeshRegistry::getOrCreate(glm::ivec3 p_chunk_pos) {
    return _meshes[makeChunkKey(p_chunk_pos)];
}

MeshBuilder* ChunkMeshRegistry::get(glm::ivec3 p_chunk_pos) {
    auto it = _meshes.find(makeChunkKey(p_chunk_pos));
    if (it == _meshes.end()) return nullptr;
    return &it->second;
}

const MeshBuilder* ChunkMeshRegistry::get(glm::ivec3 p_chunk_pos) const {
    auto it = _meshes.find(makeChunkKey(p_chunk_pos));
    if (it == _meshes.end()) return nullptr;
    return &it->second;
}

void ChunkMeshRegistry::remove(glm::ivec3 p_chunk_pos) {
    _meshes.erase(makeChunkKey(p_chunk_pos));
}

void ChunkMeshRegistry::clear() {
    _meshes.clear();
}
