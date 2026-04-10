#include "world/chunk_manager.h"

Chunk* ChunkManager::addChunk(glm::ivec3 pos) {
    std::string id = getStringFromIvec(pos);
    auto it = chunks.find(id);
    if (it != chunks.end() && it->second) return it->second.get();
    auto chunk = std::make_unique<Chunk>(pos);
    chunk->init();
    chunks[id] = std::move(chunk);
    return chunks[id].get();
}

void ChunkManager::removeChunk(glm::ivec3 pos) {
    std::string id = getStringFromIvec(pos);
    auto it = chunks.find(id);
    if (it == chunks.end() || !it->second) return;

    chunks.erase(it);
}

std::string ChunkManager::getStringFromIvec(glm::ivec3 v) {
    return std::to_string(v.x) + "," + std::to_string(v.y) + "," + std::to_string(v.z);
}

void ChunkManager::forEachChunk(const std::function<void(Chunk&)>& p_fn) {
    for (auto& [key, chunk] : chunks) {
        p_fn(*chunk);
    }
}

Chunk* ChunkManager::getChunk(glm::ivec3 pos) {
    std::string id = getStringFromIvec(pos);
    auto it = chunks.find(id);
    if (it == chunks.end() || !it->second) return nullptr;
    return it->second.get();
};

const Chunk* ChunkManager::getChunk(glm::ivec3 pos) const {
    std::string id = getStringFromIvec(pos);
    auto it = chunks.find(id);
    if (it == chunks.end() || !it->second) return nullptr;
    return it->second.get();
};
