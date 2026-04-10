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
    _chunk_render_sync.removeChunk(
        pos,
        _chunk_mesh_registry,
        _app.getChunkRenderStateCache(),
        _app.getBufferManager()
    );
    _chunk_manager.removeChunk(pos);
}

void VoxelEngine::setVoxel(glm::ivec3 chunk_pos, glm::ivec3 local_voxel_pos, Voxel voxel) {
    Chunk* chunk = _chunk_manager.getChunk(chunk_pos);
    if (!chunk) {
        chunk = _chunk_manager.addChunk(chunk_pos);
    }

    chunk->addVoxel(local_voxel_pos, voxel);
}

Voxel VoxelEngine::getVoxel(glm::ivec3 chunk_pos, glm::ivec3 local_voxel_pos) const {
    const Chunk* chunk = _chunk_manager.getChunk(chunk_pos);
    if (!chunk) {
        return Voxel(0);
    }

    return chunk->getVoxel(local_voxel_pos.x, local_voxel_pos.y, local_voxel_pos.z);
}
