#ifndef CHUNK_RENDER_SYNC_H
#define CHUNK_RENDER_SYNC_H

#include <glm/glm.hpp>

class BufferManager;
class Chunk;
class ChunkManager;
class ChunkMeshRegistry;
class ChunkRenderStateCache;

class ChunkRenderSync {
public:
    void syncChunk(
        Chunk& p_chunk,
        ChunkMeshRegistry& p_chunk_mesh_registry,
        ChunkRenderStateCache& p_chunk_render_state_cache,
        glm::vec3 p_camera_pos,
        BufferManager& p_buffer_manager
    );

    void syncAll(
        ChunkManager& p_chunk_manager,
        ChunkMeshRegistry& p_chunk_mesh_registry,
        ChunkRenderStateCache& p_chunk_render_state_cache,
        glm::vec3 p_camera_pos,
        BufferManager& p_buffer_manager
    );

    void removeChunk(
        glm::ivec3 p_chunk_pos,
        ChunkMeshRegistry& p_chunk_mesh_registry,
        ChunkRenderStateCache& p_chunk_render_state_cache,
        BufferManager& p_buffer_manager
    );
};

#endif
