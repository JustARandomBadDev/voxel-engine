#ifndef ENGINE_MESH_H
#define ENGINE_MESH_H

#include <vector>
#include <glm/glm.hpp>

#include "graphics/vertex.h"
#include "engine/face_position.h"
#include "engine/texture_data.h"

struct FaceData {
    Vertex vertex[4];
    float distance;
    glm::vec3 center;
};

class Mesh {
public:
    void add(FaceData p_face);
    virtual void add(glm::vec3 p_pos, FacePosition p_face_pos, glm::vec3 p_normal, FaceTextureData p_uv, float p_shininess);
    
    void clear();

    std::vector<Vertex>&   getVertex() { return _vertex; }
    std::vector<uint32_t>& getIndex()  { return _index; }
    virtual bool isEmpty()  { return (! _vertex.size()); }

private:
    std::vector<Vertex> _vertex;
    std::vector<uint32_t> _index;

    void addIndex();
};

#endif