#include "world/chunk.h"

#include <iostream>

#include "graphics/buffer_manager.h"
#include "world/chunk_manager.h"
#include "engine/voxel_data.h"
#include "engine/mesh_builder.h"

Chunk::Chunk()
: _opaque_alloc_id(-1), _transparent_alloc_id(-1), _is_modify(false) {}

Chunk::Chunk(glm::ivec3 p_pos)
: _pos(p_pos), _opaque_alloc_id(-1), _transparent_alloc_id(-1), _is_modify(false) {}

Chunk::~Chunk() = default;

void Chunk::init() {
    _mesh_builder = std::make_unique<MeshBuilder>();

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                _voxels[x][y][z] = Voxel(0);
            }
        }
    }
}

void Chunk::addVoxel(glm::ivec3 p_pos, Voxel p_voxel) {
    _voxels[p_pos.x][p_pos.y][p_pos.z] = p_voxel;
    _is_modify = true;
}

void Chunk::update(ChunkManager& p_chunk_manager) {
    if (!_is_modify || !_mesh_builder) return;

    _mesh_builder->buildMeshes(_voxels, _pos, p_chunk_manager);
}

void Chunk::upload(glm::vec3 p_camera_pos, BufferManager& p_buffer_manager) {
    if (!_mesh_builder) return;

    Mesh& opaque_mesh = _mesh_builder->getOpaqueMesh();
    SortableMesh& transparent_mesh = _mesh_builder->getTransparentMesh();
    if (! opaque_mesh.isEmpty() && _is_modify) _opaque_alloc_id = p_buffer_manager.getAllocator().allocMesh(opaque_mesh, _opaque_alloc_id, p_buffer_manager);
    if (! transparent_mesh.isEmpty()) {
        transparent_mesh.sort(p_camera_pos);
        _transparent_alloc_id = p_buffer_manager.getTransparentAllocator().allocMesh(transparent_mesh, _transparent_alloc_id, p_buffer_manager);
    }

    _is_modify = false;
}

void Chunk::cleanup(BufferManager& p_buffer_manager) {
    _mesh_builder.reset();
    
    if (_opaque_alloc_id != -1) {
        p_buffer_manager.getAllocator().freeMesh(_opaque_alloc_id);
        _opaque_alloc_id = -1;
    }

    if (_transparent_alloc_id != -1) {
        p_buffer_manager.getTransparentAllocator().freeMesh(_transparent_alloc_id);
        _transparent_alloc_id = -1;
    }
}
