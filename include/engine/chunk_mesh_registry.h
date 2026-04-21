#ifndef CHUNK_MESH_REGISTRY_H
#define CHUNK_MESH_REGISTRY_H

#include <unordered_map>

#include <glm/glm.hpp>

#include "core/chunk_key.h"
#include "engine/mesh_builder.h"

class ChunkMeshRegistry {
public:
    MeshBuilder& getOrCreate(glm::ivec3 p_chunk_pos);

    MeshBuilder* get(glm::ivec3 p_chunk_pos);
    const MeshBuilder* get(glm::ivec3 p_chunk_pos) const;

    void remove(glm::ivec3 p_chunk_pos);
    void clear();

private:
    std::unordered_map<ChunkKey, MeshBuilder, ChunkKeyHash> _meshes;
};

#endif
