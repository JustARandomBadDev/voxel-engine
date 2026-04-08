#ifndef CHUNK_MESH_REGISTRY_H
#define CHUNK_MESH_REGISTRY_H

#include <string>
#include <unordered_map>

#include <glm/glm.hpp>

#include "engine/mesh_builder.h"

class ChunkMeshRegistry {
public:
    MeshBuilder& getOrCreate(glm::ivec3 p_chunk_pos);
    MeshBuilder* get(glm::ivec3 p_chunk_pos);
    void remove(glm::ivec3 p_chunk_pos);
    void clear();

private:
    std::unordered_map<std::string, MeshBuilder> _meshes;

    static std::string getKey(glm::ivec3 p_chunk_pos);
};

#endif
