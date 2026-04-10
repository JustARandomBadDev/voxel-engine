#ifndef CHUNK_RENDER_STATE_H
#define CHUNK_RENDER_STATE_H

#include <string>
#include <unordered_map>

#include <glm/glm.hpp>

class BufferManager;
class Mesh;
class SortableMesh;

struct ChunkRenderState {
    int opaqueAllocId = -1;
    int transparentAllocId = -1;
};

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
    std::unordered_map<std::string, ChunkRenderState> _states;

    static std::string getKey(glm::ivec3 p_chunk_pos);
    static void freeAllocations(ChunkRenderState& p_state, BufferManager& p_buffer_manager);
    static void freeOpaqueAllocation(ChunkRenderState& p_state, BufferManager& p_buffer_manager);
    static void freeTransparentAllocation(ChunkRenderState& p_state, BufferManager& p_buffer_manager);
};

#endif
