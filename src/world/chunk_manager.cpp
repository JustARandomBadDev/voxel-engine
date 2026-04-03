#include "world/chunk_manager.h"

#include <iostream>

#include "graphics/buffer_manager.h"

Chunk* ChunkManager::addChunk(glm::ivec3 pos) {
    std::string id = getStringFromIvec(pos);
    auto it = chunks.find(id);
    if (it != chunks.end() && it->second) return it->second.get();
    auto chunk = std::make_unique<Chunk>(pos);
    chunk->init();
    chunks[id] = std::move(chunk);
    return chunks[id].get();
}

void ChunkManager::removeChunk(glm::ivec3 pos, BufferManager& p_buffer_manager) {
    std::string id = getStringFromIvec(pos);
    auto it = chunks.find(id);
    if (it == chunks.end() || !it->second) return;

    it->second->cleanup(p_buffer_manager);
    chunks.erase(it);
}

std::string ChunkManager::getStringFromIvec(glm::ivec3 v) {
    return std::to_string(v.x) + "," + std::to_string(v.y) + "," + std::to_string(v.z);
}

void ChunkManager::update() {
    for (auto& [key, chunk] : chunks) {
        chunk->update(*this);
    }
}

void ChunkManager::upload(glm::vec3 p_camera_pos, BufferManager& p_buffer_manager) {
    for (auto& [key, chunk] : chunks) {
        chunk->upload(p_camera_pos, p_buffer_manager);
    }
}

Chunk* ChunkManager::getChunk(glm::ivec3 pos) {
    std::string id = getStringFromIvec(pos);
    auto it = chunks.find(id);
    if (it == chunks.end() || !it->second) return nullptr;
    return it->second.get();
};
