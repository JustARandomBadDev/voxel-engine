#ifndef MESH_BUILDER_H
#define MESH_BUILDER_H

#include "engine/voxel_data.h"
#include "engine/sortable_mesh.h"
#include "world/voxel.h"

class ChunkManager;

class MeshBuilder {
public:
    void buildMeshes(
        Voxel (& voxels) [CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE],
        glm::ivec3 p_chunk_pos,
        ChunkManager& p_chunk_manager
    );

    Mesh& getOpaqueMesh() { return _opaque_mesh; }
    SortableMesh& getTransparentMesh() { return _transparent_mesh; }

private:
    Mesh _opaque_mesh;
    SortableMesh _transparent_mesh;
};

#endif
