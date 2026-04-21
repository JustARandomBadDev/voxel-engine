#ifndef SORTABLE_MESH_H
#define SORTABLE_MESH_H

#include "engine/mesh.h"

class SortableMesh : public Mesh {
public:
    void add(glm::vec3 p_pos, FacePosition p_face_pos, glm::vec3 p_normal, FaceTextureData p_uv, float p_shininess) override;
    void clear();
    void sort(glm::vec3 p_camera_pos);

    bool isEmpty() const { return Mesh::isEmpty() && ! _faces.size(); }

private:
    std::vector<FaceData> _faces;
};

#endif
