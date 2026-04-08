#ifndef CHUNK_NEIGHBORHOOD_VIEW_H
#define CHUNK_NEIGHBORHOOD_VIEW_H

#include <glm/glm.hpp>

class Chunk;
class ChunkManager;

class ChunkNeighborhoodView {
public:
    explicit ChunkNeighborhoodView(ChunkManager& p_chunk_manager);

    const Chunk* getChunk(glm::ivec3 p_pos) const;

private:
    ChunkManager& _chunk_manager;
};

#endif
