#ifndef CHUNK_MANAGER_H
#define CHUNK_MANAGER_H

#include <functional>
#include <memory>
#include <unordered_map>

#include <glm/glm.hpp>

#include "core/chunk_key.h"
#include "world/chunk.h"

class ChunkManager {
public:
    Chunk* addChunk(glm::ivec3 pos);
    void removeChunk(glm::ivec3 pos);

    Chunk* getChunk(glm::ivec3 pos);
    const Chunk* getChunk(glm::ivec3 pos) const;
    void forEachChunk(const std::function<void(Chunk&)>& p_fn);

private:
    std::unordered_map<ChunkKey, std::unique_ptr<Chunk>, ChunkKeyHash> chunks;
};

#endif
