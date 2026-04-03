#ifndef CHUNK_MANAGER_H
#define CHUNK_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

#include "world/chunk.h"

class BufferManager;

class ChunkManager {
public:
    Chunk* addChunk(glm::ivec3 pos);
    void removeChunk(glm::ivec3 pos, BufferManager& p_buffer_manager);

    void update();
    void upload(glm::vec3 p_camera_pos, BufferManager& p_buffer_manager);

    static std::string getStringFromIvec(glm::ivec3 v);

    Chunk* getChunk(glm::ivec3 pos);

private:
    std::unordered_map<std::string, std::unique_ptr<Chunk>> chunks;
};

#endif
