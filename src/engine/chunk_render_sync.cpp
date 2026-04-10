#include "engine/chunk_render_sync.h"

#include "engine/chunk_mesh_registry.h"
#include "graphics/buffer_manager.h"
#include "graphics/chunk_render_state.h"
#include "world/chunk.h"
#include "world/chunk_manager.h"

void ChunkRenderSync::syncChunk(
    Chunk& p_chunk,
    ChunkMeshRegistry& p_chunk_mesh_registry,
    ChunkRenderStateCache& p_chunk_render_state_cache,
    glm::vec3 p_camera_pos,
    BufferManager& p_buffer_manager
) {
    MeshBuilder* mesh_builder = p_chunk_mesh_registry.get(p_chunk.getPos());
    if (!mesh_builder) return;

    p_chunk_render_state_cache.upload(
        p_chunk.getPos(),
        mesh_builder->getOpaqueMesh(),
        mesh_builder->getTransparentMesh(),
        p_chunk.isDirty(),
        p_camera_pos,
        p_buffer_manager
    );

    p_chunk.clearDirty();
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

void ChunkRenderSync::removeChunk(
    glm::ivec3 p_chunk_pos,
    ChunkMeshRegistry& p_chunk_mesh_registry,
    ChunkRenderStateCache& p_chunk_render_state_cache,
    BufferManager& p_buffer_manager
) {
    p_chunk_mesh_registry.remove(p_chunk_pos);
    p_chunk_render_state_cache.remove(p_chunk_pos, p_buffer_manager);
}
