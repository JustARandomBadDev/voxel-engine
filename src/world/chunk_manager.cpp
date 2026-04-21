#include "world/chunk_manager.h"

Chunk* ChunkManager::addChunk(glm::ivec3 pos) {
    const ChunkKey key = makeChunkKey(pos);
    auto it = chunks.find(key);
    if (it != chunks.end() && it->second) return it->second.get();
    auto chunk = std::make_unique<Chunk>(pos);
    chunk->init();
    auto [insertedIt, inserted] = chunks.emplace(key, std::move(chunk));
    (void)inserted;
    return insertedIt->second.get();
}

void ChunkManager::removeChunk(glm::ivec3 pos) {
    auto it = chunks.find(makeChunkKey(pos));
    if (it == chunks.end() || !it->second) return;

    chunks.erase(it);
}

void ChunkManager::forEachChunk(const std::function<void(Chunk&)>& p_fn) {
    for (auto& chunk_entry : chunks) {
        p_fn(*chunk_entry.second);
    }
}

Chunk* ChunkManager::getChunk(glm::ivec3 pos) {
    auto it = chunks.find(makeChunkKey(pos));
    if (it == chunks.end() || !it->second) return nullptr;
    return it->second.get();
}

const Chunk* ChunkManager::getChunk(glm::ivec3 pos) const {
    auto it = chunks.find(makeChunkKey(pos));
    if (it == chunks.end() || !it->second) return nullptr;
    return it->second.get();
}
