#ifndef CHUNK_RENDER_STATE_H
#define CHUNK_RENDER_STATE_H

#include <unordered_map>

#include <glm/glm.hpp>

#include "core/chunk_key.h"

class BufferManager;
class Mesh;
class SortableMesh;

// Stores allocator-managed logical allocation ids for opaque/transparent mesh data plus the last camera position used to sort transparent data.
struct ChunkRenderState {
    int opaqueAllocId = -1;
    int transparentAllocId = -1;
    glm::vec3 lastTransparentCameraPos = {0.0f, 0.0f, 0.0f};
    bool hasTransparentCameraPos = false;
};

// Maps chunk positions to allocator-managed GPU render state for opaque and transparent chunk meshes.
class ChunkRenderStateCache {
public:
    void upload(
        glm::ivec3 p_chunk_pos,
        Mesh& p_opaque_mesh,
        SortableMesh& p_transparent_mesh,
        bool p_chunk_is_dirty,
        glm::vec3 p_camera_pos,
        BufferManager& p_buffer_manager
    );
    void remove(glm::ivec3 p_chunk_pos, BufferManager& p_buffer_manager);
    void cleanup(BufferManager& p_buffer_manager);

private:
    std::unordered_map<ChunkKey, ChunkRenderState, ChunkKeyHash> _states;

    static void freeAllocations(ChunkRenderState& p_state, BufferManager& p_buffer_manager);
    static void freeOpaqueAllocation(ChunkRenderState& p_state, BufferManager& p_buffer_manager);
    static void freeTransparentAllocation(ChunkRenderState& p_state, BufferManager& p_buffer_manager);
};

#endif
