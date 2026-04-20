#include "engine/chunk_mesh_registry.h"
#include "engine/mesh_builder.h"

std::string ChunkMeshRegistry::getKey(glm::ivec3 p_chunk_pos) {
    return std::to_string(p_chunk_pos.x) + "," + std::to_string(p_chunk_pos.y) + "," + std::to_string(p_chunk_pos.z);
}

MeshBuilder& ChunkMeshRegistry::getOrCreate(glm::ivec3 p_chunk_pos) {
    return _meshes[getKey(p_chunk_pos)];
}

MeshBuilder* ChunkMeshRegistry::get(glm::ivec3 p_chunk_pos) {
    auto it = _meshes.find(getKey(p_chunk_pos));
    if (it == _meshes.end()) return nullptr;
    return &it->second;
}

const MeshBuilder* ChunkMeshRegistry::get(glm::ivec3 p_chunk_pos) const {
    auto it = _meshes.find(getKey(p_chunk_pos));
    if (it == _meshes.end()) return nullptr;
    return &it->second;
}

void ChunkMeshRegistry::remove(glm::ivec3 p_chunk_pos) {
    _meshes.erase(getKey(p_chunk_pos));
}

void ChunkMeshRegistry::clear() {
    _meshes.clear();
}
