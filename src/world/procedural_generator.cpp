#include "world/procedural_generator.h"

#include "core/config.h"

ProceduralGenerator::ProceduralGenerator(ChunkManager& p_chunk_manager)
: chunk_manager(p_chunk_manager) {
    perlin_noise = PerlinNoise2D();
}

void ProceduralGenerator::generateChunk(glm::vec2 pos) {
    std::unordered_map<int, Chunk*> local_chunks;

    auto getOrCreateChunk = [&](int y_index) -> Chunk* {
        if (local_chunks.count(y_index)) return local_chunks[y_index];
        Chunk* c = chunk_manager.getChunk({pos.x, y_index, pos.y});
        if (!c) c = chunk_manager.addChunk({pos.x, y_index, pos.y});
        return local_chunks[y_index] = c;
    };

    float frequencies[PROCEDURAL_OCTAVES];
    float amplitudes[PROCEDURAL_OCTAVES];

    for (int i = 0; i < PROCEDURAL_OCTAVES; i++) {
        frequencies[i] = PROCEDURAL_FREQUENCY * static_cast<float>(std::pow(PROCEDURAL_MULT_FREQUENCY, i));
        amplitudes[i] = PROCEDURAL_AMPLITUDE * static_cast<float>(std::pow(PROCEDURAL_PERSISTENCE, i));
    }

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            float total = 0.0f;

            for (int i = 0; i < PROCEDURAL_OCTAVES; i++) {
                float fx = (pos.x * CHUNK_SIZE + x) * frequencies[i];
                float fz = (pos.y * CHUNK_SIZE + z) * frequencies[i];
                total += perlin_noise.noise_2d(fx, fz) * amplitudes[i];
            }

            int height = static_cast<int>(total);

            int voxel_start = 0;
            int voxel_end = 0;

            if (height <= 50) {
                voxel_end = height - 5;
                for (int y = voxel_start; y < voxel_end; ++y)
                    getOrCreateChunk(y / CHUNK_SIZE)->addVoxel({x, y % CHUNK_SIZE, z}, 3); // stone

                voxel_start = voxel_end;
                voxel_end = height + 1;
                for (int y = voxel_start; y < voxel_end; ++y)
                    getOrCreateChunk(y / CHUNK_SIZE)->addVoxel({x, y % CHUNK_SIZE, z}, 9); // dirt

                voxel_start = voxel_end;
                voxel_end = 48;
                for (int y = voxel_start; y < voxel_end; ++y)
                    getOrCreateChunk(y / CHUNK_SIZE)->addVoxel({x, y % CHUNK_SIZE, z}, 8); // water
            } else {
                voxel_end = height - 2;
                for (int y = voxel_start; y < voxel_end; ++y)
                    getOrCreateChunk(y / CHUNK_SIZE)->addVoxel({x, y % CHUNK_SIZE, z}, 3); // stone

                voxel_start = voxel_end;
                voxel_end = height;
                for (int y = voxel_start; y < voxel_end; ++y)
                    getOrCreateChunk(y / CHUNK_SIZE)->addVoxel({x, y % CHUNK_SIZE, z}, 2); // dirt

                getOrCreateChunk(height / CHUNK_SIZE)->addVoxel({x, height % CHUNK_SIZE, z}, 1); // grass
            }
        }
    }
}
