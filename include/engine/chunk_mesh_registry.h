#ifndef CHUNK_MESH_REGISTRY_H
#define CHUNK_MESH_REGISTRY_H

#include <unordered_map>

#include <glm/glm.hpp>

#include "core/chunk_key.h"
#include "engine/mesh_builder.h"

// CPU-side mesh cache keyed by chunk position.
// It is the handoff point between meshing and render sync, not an owner of world chunks or GPU resources.
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
