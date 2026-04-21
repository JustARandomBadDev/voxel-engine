#include "engine/voxel_engine.h"

void VoxelEngine::init(const VoxelEngineInitConfig& config) {
    _app.init(config);
}

void VoxelEngine::update() {
    _chunk_mesher.updateAll(_chunk_manager, _chunk_mesh_registry);
    _chunk_render_sync.syncAll(
        _chunk_manager,
        _chunk_mesh_registry,
        _app.getChunkRenderStateCache(),
        _app.getCamera()->getPosition(),
        _app.getBufferManager()
    );
}

void VoxelEngine::render() {
    _app.getRenderer().resetCommandBuffers();
    _app.render();
}

void VoxelEngine::shutdown() {
    _app.cleanup();
}

bool VoxelEngine::isRun() const {
    return _app.isRun();
}

void VoxelEngine::createChunk(glm::ivec3 pos) {
    _chunk_manager.addChunk(pos);
}

void VoxelEngine::removeChunk(glm::ivec3 pos) {
    markAdjacentChunksDirty(pos);
    _chunk_render_sync.removeChunk(
        pos,
        _chunk_mesh_registry,
        _app.getChunkRenderStateCache(),
        _app.getBufferManager()
    );
    _chunk_manager.removeChunk(pos);
}

void VoxelEngine::setVoxel(glm::ivec3 chunk_pos, glm::ivec3 local_voxel_pos, Voxel voxel) {
    if (!checkLocalPos(local_voxel_pos)) return;
    
    if (voxel.id <= 0 || static_cast<size_t>(voxel.id - 1) >= VOXEL_DATAS.size()) return;
    
    Chunk* chunk = _chunk_manager.getChunk(chunk_pos);
    if (!chunk) {
        chunk = _chunk_manager.addChunk(chunk_pos);
    }

    chunk->addVoxel(local_voxel_pos, voxel);
    markNeighborChunksDirty(chunk_pos, local_voxel_pos);
}

Voxel VoxelEngine::getVoxel(glm::ivec3 chunk_pos, glm::ivec3 local_voxel_pos) const {
    if (!checkLocalPos(local_voxel_pos)) return Voxel(0);

    const Chunk* chunk = _chunk_manager.getChunk(chunk_pos);
    if (!chunk) {
        return Voxel(0);
    }

    return chunk->getVoxel(local_voxel_pos.x, local_voxel_pos.y, local_voxel_pos.z);
}

void VoxelEngine::markChunkDirtyIfLoaded(glm::ivec3 chunk_pos) {
    Chunk* chunk = _chunk_manager.getChunk(chunk_pos);
    if (chunk) chunk->markDirty();
}

void VoxelEngine::markNeighborChunksDirty(glm::ivec3 chunk_pos, glm::ivec3 local_voxel_pos) {
    if (local_voxel_pos.x == 0)                   markChunkDirtyIfLoaded({chunk_pos.x - 1, chunk_pos.y, chunk_pos.z});
    else if (local_voxel_pos.x == CHUNK_SIZE - 1) markChunkDirtyIfLoaded({chunk_pos.x + 1, chunk_pos.y, chunk_pos.z});

    if (local_voxel_pos.y == 0)                   markChunkDirtyIfLoaded({chunk_pos.x, chunk_pos.y - 1, chunk_pos.z});
    else if (local_voxel_pos.y == CHUNK_SIZE - 1) markChunkDirtyIfLoaded({chunk_pos.x, chunk_pos.y + 1, chunk_pos.z});

    if (local_voxel_pos.z == 0)                   markChunkDirtyIfLoaded({chunk_pos.x, chunk_pos.y, chunk_pos.z - 1});
    else if (local_voxel_pos.z == CHUNK_SIZE - 1) markChunkDirtyIfLoaded({chunk_pos.x, chunk_pos.y, chunk_pos.z + 1});
}

void VoxelEngine::markAdjacentChunksDirty(glm::ivec3 chunk_pos) {
    markChunkDirtyIfLoaded({chunk_pos.x - 1, chunk_pos.y, chunk_pos.z});
    markChunkDirtyIfLoaded({chunk_pos.x + 1, chunk_pos.y, chunk_pos.z});
    markChunkDirtyIfLoaded({chunk_pos.x, chunk_pos.y - 1, chunk_pos.z});
    markChunkDirtyIfLoaded({chunk_pos.x, chunk_pos.y + 1, chunk_pos.z});
    markChunkDirtyIfLoaded({chunk_pos.x, chunk_pos.y, chunk_pos.z - 1});
    markChunkDirtyIfLoaded({chunk_pos.x, chunk_pos.y, chunk_pos.z + 1});
}
