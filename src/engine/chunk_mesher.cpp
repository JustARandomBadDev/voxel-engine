#include "engine/chunk_mesher.h"

#include "engine/chunk_mesh_registry.h"
#include "world/chunk.h"
#include "world/chunk_manager.h"

void ChunkMesher::updateChunk(Chunk& p_chunk, ChunkManager& p_chunk_manager, ChunkMeshRegistry& p_chunk_mesh_registry) {
    if (!p_chunk.needsRenderSync()) return;

    MeshBuilder& mesh_builder = p_chunk_mesh_registry.getOrCreate(p_chunk.getPos());
    mesh_builder.buildMeshes(p_chunk.getVoxels(), p_chunk.getPos(), p_chunk_manager);
}

void ChunkMesher::updateAll(ChunkManager& p_chunk_manager, ChunkMeshRegistry& p_chunk_mesh_registry) {
    p_chunk_manager.forEachChunk([&](Chunk& p_chunk) {
        updateChunk(p_chunk, p_chunk_manager, p_chunk_mesh_registry);
    });
}
