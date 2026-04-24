#include "graphics/chunk_render_sync.h"

#include "engine/chunk_mesh_registry.h"
#include "graphics/buffer_manager.h"
#include "graphics/chunk_render_state.h"
#include "world/chunk.h"
#include "world/chunk_manager.h"

// Consumes the CPU mesh cache for one chunk, updates graphics-side render state,
// and only then clears the chunk's render-sync state.
void ChunkRenderSync::syncChunk(
    Chunk& p_chunk,
    ChunkMeshRegistry& p_chunk_mesh_registry,
    ChunkRenderStateCache& p_chunk_render_state_cache,
    glm::vec3 p_camera_pos,
    BufferManager& p_buffer_manager
) {
    MeshBuilder* mesh_builder = p_chunk_mesh_registry.get(p_chunk.getPos());
    // Render sync is skipped until meshing has produced CPU mesh data for this chunk.
    // This relies on VoxelEngine::update() running meshing before render sync.
    if (!mesh_builder) return;

    p_chunk_render_state_cache.upload(
        p_chunk.getPos(),
        mesh_builder->getOpaqueMesh(),
        mesh_builder->getTransparentMesh(),
        p_chunk.needsRenderSync(),
        p_camera_pos,
        p_buffer_manager
    );
    p_chunk.clearRenderSyncDirty();
}

void ChunkRenderSync::syncAll(
    ChunkManager& p_chunk_manager,
    ChunkMeshRegistry& p_chunk_mesh_registry,
    ChunkRenderStateCache& p_chunk_render_state_cache,
    glm::vec3 p_camera_pos,
    BufferManager& p_buffer_manager
) {
    p_chunk_manager.forEachChunk([&](Chunk& p_chunk) {
        syncChunk(
            p_chunk,
            p_chunk_mesh_registry,
            p_chunk_render_state_cache,
            p_camera_pos,
            p_buffer_manager
        );
    });
}

// Removing a chunk must clear both the CPU mesh cache entry and the graphics-side
// render state/allocation tracking associated with that chunk.
void ChunkRenderSync::removeChunk(
    glm::ivec3 p_chunk_pos,
    ChunkMeshRegistry& p_chunk_mesh_registry,
    ChunkRenderStateCache& p_chunk_render_state_cache,
    BufferManager& p_buffer_manager
) {
    p_chunk_mesh_registry.remove(p_chunk_pos);
    p_chunk_render_state_cache.remove(p_chunk_pos, p_buffer_manager);
}
