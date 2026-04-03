#include "engine/mesh.h"

#include <iostream>

#include "graphics/buffer_manager.h"
#include "engine/voxel_data.h"
#include "world/chunk.h"
#include "world/chunk_manager.h"

void Mesh::add(FaceData p_face) {
    addIndex();

    _vertex.insert( _vertex.end(), {
        p_face.vertex[0],
        p_face.vertex[1],
        p_face.vertex[2],
        p_face.vertex[3]
    });
}

void Mesh::add(glm::vec3 p_pos, FacePosition p_face_pos, glm::vec3 p_normal, FaceTextureData p_uv, float p_shininess) {
    addIndex();

    _vertex.insert( _vertex.end(), {
        {p_pos + p_face_pos.v[0], p_normal, p_uv.v[0], p_shininess},
        {p_pos + p_face_pos.v[1], p_normal, p_uv.v[1], p_shininess},
        {p_pos + p_face_pos.v[2], p_normal, p_uv.v[2], p_shininess},
        {p_pos + p_face_pos.v[3], p_normal, p_uv.v[3], p_shininess}}
    );
}

void Mesh::addIndex() {
    uint32_t index_offset = _vertex.size();

    _index.insert(_index.end(), {
        index_offset + 0,
        index_offset + 1,
        index_offset + 2,
        index_offset + 2,
        index_offset + 3,
        index_offset + 0}
    );
}

void Mesh::clear() {
    _vertex.clear();
    _index.clear();
}