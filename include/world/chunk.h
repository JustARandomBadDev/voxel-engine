#ifndef CHUNK_H
#define CHUNK_H

#include <memory>

#include <glm/glm.hpp>

#include "core/config.h"
#include "engine/sortable_mesh.h"
#include "world/voxel.h"
#include "graphics/allocator.h"

class MeshBuilder;
class ChunkManager;
class BufferManager;

class Chunk {
public:
    Chunk();
    Chunk(glm::ivec3 p_pos);
    ~Chunk();

    void init();
    void addVoxel(glm::ivec3 p_pos, Voxel p_voxel);
    void removeVoxel(glm::ivec3 p_pos) { addVoxel(p_pos, Voxel(0)); };
    void update(ChunkManager& p_chunk_manager);
    void upload(glm::vec3 p_camera_pos, BufferManager& p_buffer_manager);
    void cleanup(BufferManager& p_buffer_manager);

    glm::ivec3 getPos()                      { return _pos; }
    int        getOpaqueAllocId()            { return _opaque_alloc_id; }
    int        getTransparentAllocId()       { return _transparent_alloc_id; }
    Voxel      getVoxel(int x, int y, int z) { return _voxels[x][y][z]; }
    
    Voxel getVoxel(glm::vec3 p_pos) {
        return getVoxel(
            static_cast<int>(p_pos.x),
            static_cast<int>(p_pos.y),
            static_cast<int>(p_pos.z)
        );
    }

private:
    glm::ivec3 _pos;

    Voxel _voxels[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];

    std::unique_ptr<MeshBuilder> _mesh_builder;

    int _opaque_alloc_id;
    int _transparent_alloc_id;

    bool _is_modify;
};

#endif
