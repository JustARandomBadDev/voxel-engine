#include "graphics/chunk_render_state.h"

#include <glm/geometric.hpp>

#include "engine/mesh.h"
#include "engine/sortable_mesh.h"
#include "graphics/buffer_manager.h"

namespace {
constexpr float kTransparentResortCameraDistance = 1.0f;
constexpr float kTransparentResortCameraDistanceSquared =
    kTransparentResortCameraDistance * kTransparentResortCameraDistance;

bool shouldResortTransparentMesh(
    const ChunkRenderState& p_state,
    bool p_chunk_is_dirty,
    glm::vec3 p_camera_pos
) {
    if (p_chunk_is_dirty) return true;
    if (p_state.transparentAllocId == -1) return true;
    if (!p_state.hasTransparentCameraPos) return true;

    const glm::vec3 cameraDelta = p_state.lastTransparentCameraPos - p_camera_pos;
    return glm::dot(cameraDelta, cameraDelta) >= kTransparentResortCameraDistanceSquared;
}
} // namespace

std::string ChunkRenderStateCache::getKey(glm::ivec3 p_chunk_pos) {
    return std::to_string(p_chunk_pos.x) + "," + std::to_string(p_chunk_pos.y) + "," + std::to_string(p_chunk_pos.z);
}

void ChunkRenderStateCache::freeAllocations(ChunkRenderState& p_state, BufferManager& p_buffer_manager) {
    freeOpaqueAllocation(p_state, p_buffer_manager);
    freeTransparentAllocation(p_state, p_buffer_manager);
}

void ChunkRenderStateCache::freeOpaqueAllocation(ChunkRenderState& p_state, BufferManager& p_buffer_manager) {
    if (p_state.opaqueAllocId != -1) {
        p_buffer_manager.getAllocator().freeMesh(p_state.opaqueAllocId, p_buffer_manager);
        p_state.opaqueAllocId = -1;
    }
}

void ChunkRenderStateCache::freeTransparentAllocation(ChunkRenderState& p_state, BufferManager& p_buffer_manager) {
    if (p_state.transparentAllocId != -1) {
        p_buffer_manager.getTransparentAllocator().freeMesh(p_state.transparentAllocId, p_buffer_manager);
        p_state.transparentAllocId = -1;
    }

    p_state.hasTransparentCameraPos = false;
    p_state.lastTransparentCameraPos = {0.0f, 0.0f, 0.0f};
}

void ChunkRenderStateCache::upload(
    glm::ivec3 p_chunk_pos,
    Mesh& p_opaque_mesh,
    SortableMesh& p_transparent_mesh,
    bool p_chunk_is_dirty,
    glm::vec3 p_camera_pos,
    BufferManager& p_buffer_manager
) {
    ChunkRenderState& state = _states[getKey(p_chunk_pos)];

    if (!p_opaque_mesh.isEmpty()) {
        if (p_chunk_is_dirty) {
            
            state.opaqueAllocId = p_buffer_manager.getAllocator().allocMesh(
                p_opaque_mesh,
                state.opaqueAllocId,
                p_buffer_manager
            );
        }   
    } else freeOpaqueAllocation(state, p_buffer_manager);

    if (!p_transparent_mesh.isEmpty()) {
        const bool mustUpdateTransparent = shouldResortTransparentMesh(
            state,
            p_chunk_is_dirty,
            p_camera_pos
        );

        if (mustUpdateTransparent) {
            p_transparent_mesh.sort(p_camera_pos);
            state.transparentAllocId = p_buffer_manager.getTransparentAllocator().allocMesh(
                p_transparent_mesh,
                state.transparentAllocId,
                p_buffer_manager
            );
            state.lastTransparentCameraPos = p_camera_pos;
            state.hasTransparentCameraPos = true;
        }
    } else freeTransparentAllocation(state, p_buffer_manager);
}

void ChunkRenderStateCache::remove(glm::ivec3 p_chunk_pos, BufferManager& p_buffer_manager) {
    auto it = _states.find(getKey(p_chunk_pos));
    if (it == _states.end()) return;

    freeAllocations(it->second, p_buffer_manager);
    _states.erase(it);
}

void ChunkRenderStateCache::cleanup(BufferManager& p_buffer_manager) {
    for (auto& [key, state] : _states) {
        freeAllocations(state, p_buffer_manager);
    }

    _states.clear();
}
