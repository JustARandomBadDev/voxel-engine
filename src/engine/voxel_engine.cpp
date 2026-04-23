#include "engine/voxel_engine.h"

#include <memory>

#include "core/config.h"
#include "engine/chunk_mesh_registry.h"
#include "engine/chunk_mesher.h"
#include "engine/voxel_data.h"
#include "graphics/graphics_runtime.h"
#include "graphics/chunk_render_sync.h"
#include "world/chunk_manager.h"

namespace {

bool checkLocalPos(glm::ivec3 local_voxel_pos) {
    return local_voxel_pos.x >= 0 && local_voxel_pos.x < CHUNK_SIZE &&
           local_voxel_pos.y >= 0 && local_voxel_pos.y < CHUNK_SIZE &&
           local_voxel_pos.z >= 0 && local_voxel_pos.z < CHUNK_SIZE;
}

} // namespace

class VoxelEngine::Impl {
public:
    GraphicsRuntime app;
    ChunkManager chunk_manager;
    ChunkMesher chunk_mesher;
    ChunkMeshRegistry chunk_mesh_registry;
    ChunkRenderSync chunk_render_sync;

    void markChunkDirtyIfLoaded(glm::ivec3 chunk_pos) {
        Chunk* chunk = chunk_manager.getChunk(chunk_pos);
        if (chunk) chunk->markRenderSyncDirty();
    }

    void markNeighborChunksDirty(glm::ivec3 chunk_pos, glm::ivec3 local_voxel_pos) {
        if (local_voxel_pos.x == 0)                   markChunkDirtyIfLoaded({chunk_pos.x - 1, chunk_pos.y, chunk_pos.z});
        else if (local_voxel_pos.x == CHUNK_SIZE - 1) markChunkDirtyIfLoaded({chunk_pos.x + 1, chunk_pos.y, chunk_pos.z});

        if (local_voxel_pos.y == 0)                   markChunkDirtyIfLoaded({chunk_pos.x, chunk_pos.y - 1, chunk_pos.z});
        else if (local_voxel_pos.y == CHUNK_SIZE - 1) markChunkDirtyIfLoaded({chunk_pos.x, chunk_pos.y + 1, chunk_pos.z});

        if (local_voxel_pos.z == 0)                   markChunkDirtyIfLoaded({chunk_pos.x, chunk_pos.y, chunk_pos.z - 1});
        else if (local_voxel_pos.z == CHUNK_SIZE - 1) markChunkDirtyIfLoaded({chunk_pos.x, chunk_pos.y, chunk_pos.z + 1});
    }

    void markAdjacentChunksDirty(glm::ivec3 chunk_pos) {
        markChunkDirtyIfLoaded({chunk_pos.x - 1, chunk_pos.y, chunk_pos.z});
        markChunkDirtyIfLoaded({chunk_pos.x + 1, chunk_pos.y, chunk_pos.z});
        markChunkDirtyIfLoaded({chunk_pos.x, chunk_pos.y - 1, chunk_pos.z});
        markChunkDirtyIfLoaded({chunk_pos.x, chunk_pos.y + 1, chunk_pos.z});
        markChunkDirtyIfLoaded({chunk_pos.x, chunk_pos.y, chunk_pos.z - 1});
        markChunkDirtyIfLoaded({chunk_pos.x, chunk_pos.y, chunk_pos.z + 1});
    }
};

VoxelEngine::VoxelEngine()
: _impl(std::make_unique<Impl>()) {}

VoxelEngine::~VoxelEngine() = default;

VoxelEngine::VoxelEngine(VoxelEngine&&) noexcept = default;

VoxelEngine& VoxelEngine::operator=(VoxelEngine&&) noexcept = default;

void VoxelEngine::init(const VoxelEngineInitConfig& config) {
    _impl->app.init(config);
}

void VoxelEngine::update(const Camera& camera) {
    _impl->chunk_mesher.updateAll(_impl->chunk_manager, _impl->chunk_mesh_registry);
    _impl->chunk_render_sync.syncAll(
        _impl->chunk_manager,
        _impl->chunk_mesh_registry,
        _impl->app.getChunkRenderStateCache(),
        camera.getPosition(),
        _impl->app.getBufferManager()
    );
}

void VoxelEngine::render(const Camera& camera) {
    _impl->app.render(camera);
}

void VoxelEngine::shutdown() {
    _impl->app.cleanup();
}

float VoxelEngine::getAspectRatio() const {
    return _impl->app.getAspectRatio();
}

void VoxelEngine::createChunk(glm::ivec3 pos) {
    _impl->chunk_manager.addChunk(pos);
}

void VoxelEngine::removeChunk(glm::ivec3 pos) {
    _impl->markAdjacentChunksDirty(pos);
    _impl->chunk_render_sync.removeChunk(
        pos,
        _impl->chunk_mesh_registry,
        _impl->app.getChunkRenderStateCache(),
        _impl->app.getBufferManager()
    );
    _impl->chunk_manager.removeChunk(pos);
}

void VoxelEngine::setVoxel(glm::ivec3 chunk_pos, glm::ivec3 local_voxel_pos, Voxel voxel) {
    if (!checkLocalPos(local_voxel_pos)) return;

    if (voxel.id == 0) {
        removeVoxel(chunk_pos, local_voxel_pos);
        return;
    }

    if (static_cast<size_t>(voxel.id - 1) >= VOXEL_DATAS.size()) return;

    Chunk* chunk = _impl->chunk_manager.getChunk(chunk_pos);
    if (!chunk) {
        chunk = _impl->chunk_manager.addChunk(chunk_pos);
    }

    chunk->addVoxel(local_voxel_pos, voxel);
    _impl->markNeighborChunksDirty(chunk_pos, local_voxel_pos);
}

void VoxelEngine::removeVoxel(glm::ivec3 chunk_pos, glm::ivec3 local_voxel_pos) {
    if (!checkLocalPos(local_voxel_pos)) return;

    Chunk* chunk = _impl->chunk_manager.getChunk(chunk_pos);
    if (!chunk) return;

    chunk->removeVoxel(local_voxel_pos);
    _impl->markNeighborChunksDirty(chunk_pos, local_voxel_pos);
}

Voxel VoxelEngine::getVoxel(glm::ivec3 chunk_pos, glm::ivec3 local_voxel_pos) const {
    if (!checkLocalPos(local_voxel_pos)) return Voxel(0);

    const Chunk* chunk = _impl->chunk_manager.getChunk(chunk_pos);
    if (!chunk) {
        return Voxel(0);
    }

    return chunk->getVoxel(local_voxel_pos.x, local_voxel_pos.y, local_voxel_pos.z);
}
