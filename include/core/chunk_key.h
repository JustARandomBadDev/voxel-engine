#ifndef CHUNK_KEY_H
#define CHUNK_KEY_H

#include <cstddef>
#include <functional>

#include <glm/glm.hpp>

struct ChunkKey {
    int x;
    int y;
    int z;

    bool operator==(const ChunkKey& p_other) const {
        return x == p_other.x && y == p_other.y && z == p_other.z;
    }
};

struct ChunkKeyHash {
    std::size_t operator()(const ChunkKey& p_key) const noexcept {
        std::size_t seed = std::hash<int>{}(p_key.x);
        seed ^= std::hash<int>{}(p_key.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>{}(p_key.z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

inline ChunkKey makeChunkKey(glm::ivec3 p_chunk_pos) noexcept {
    return {p_chunk_pos.x, p_chunk_pos.y, p_chunk_pos.z};
}

#endif
