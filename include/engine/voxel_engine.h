#ifndef VOXEL_ENGINE_H
#define VOXEL_ENGINE_H

#include "engine/chunk_mesh_registry.h"
#include "engine/chunk_mesher.h"
#include "engine/chunk_render_sync.h"
#include "engine/voxel_engine_config.h"
#include "graphics/app.h"
#include "world/chunk_manager.h"
#include "world/voxel.h"

class VoxelEngine {
public:
    void init(const VoxelEngineInitConfig& config);
    void update();
    void render();
    void shutdown();
    bool isRun() const;
    void createChunk(glm::ivec3 pos);
    void removeChunk(glm::ivec3 pos);
    void setVoxel(glm::ivec3 chunk_pos, glm::ivec3 local_voxel_pos, Voxel voxel);
    Voxel getVoxel(glm::ivec3 chunk_pos, glm::ivec3 local_voxel_pos) const;

private:
    VulkanApp _app;
    ChunkManager _chunk_manager;
    ChunkMesher _chunk_mesher;
    ChunkMeshRegistry _chunk_mesh_registry;
    ChunkRenderSync _chunk_render_sync;
};

#endif
