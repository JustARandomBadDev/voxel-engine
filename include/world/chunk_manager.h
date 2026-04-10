#ifndef CHUNK_MANAGER_H
#define CHUNK_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <glm/glm.hpp>

#include "world/chunk.h"

class ChunkManager {
public:
    Chunk* addChunk(glm::ivec3 pos);
    void removeChunk(glm::ivec3 pos);

    static std::string getStringFromIvec(glm::ivec3 v);

    Chunk* getChunk(glm::ivec3 pos);
    const Chunk* getChunk(glm::ivec3 pos) const;
    void forEachChunk(const std::function<void(Chunk&)>& p_fn);

private:
    std::unordered_map<std::string, std::unique_ptr<Chunk>> chunks;
};

#endif
