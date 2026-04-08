#include "engine/chunk_neighborhood_view.h"

#include "world/chunk_manager.h"

ChunkNeighborhoodView::ChunkNeighborhoodView(ChunkManager& p_chunk_manager)
: _chunk_manager(p_chunk_manager) {}

Chunk* ChunkNeighborhoodView::getChunk(glm::ivec3 p_pos) const {
    return _chunk_manager.getChunk(p_pos);
}
