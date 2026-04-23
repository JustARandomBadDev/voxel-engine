#ifndef ENGINE_MESH_H
#define ENGINE_MESH_H

#include <vector>
#include <glm/glm.hpp>

#include "graphics/vertex.h"
#include "engine/face_position.h"
#include "engine/texture_data.h"

// One quad worth of geometry, with optional sort metadata used by transparent mesh handling.
struct FaceData {
    Vertex vertex[4];
    float distance;
    glm::vec3 center;
};

// CPU-side vertex/index data for one chunk mesh stream.
// This type owns no GPU resources.
class Mesh {
public:
    void add(FaceData p_face);
    virtual void add(glm::vec3 p_pos, FacePosition p_face_pos, glm::vec3 p_normal, FaceTextureData p_uv, float p_shininess);
    
    void clear();

    const std::vector<Vertex>&   getVertex() const { return _vertex; }
    const std::vector<uint32_t>& getIndex()  const { return _index; }
    bool isEmpty() const { return (! _vertex.size()); }

private:
    std::vector<Vertex> _vertex;
    std::vector<uint32_t> _index;

    void addIndex();
};

#endif
