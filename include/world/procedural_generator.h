#ifndef PROCEDURAL_GENERATOR_HPP
#define PROCEDURAL_GENERATOR_HPP

#include <math.h>

#include "world/chunk_manager.h"
#include "world/perlin_noise_2d.h"


inline constexpr int   PROCEDURAL_OCTAVES = 2;
inline constexpr float PROCEDURAL_FREQUENCY = 0.0054f;;
inline constexpr float PROCEDURAL_AMPLITUDE = 128.f;;
inline constexpr float PROCEDURAL_PERSISTENCE = 0.1f;;
inline constexpr float PROCEDURAL_MULT_FREQUENCY = 12.f;;


class ProceduralGenerator
{
    private:
        ChunkManager& chunk_manager;
        PerlinNoise2D perlin_noise;
    
    public:
        ProceduralGenerator(ChunkManager& p_chunk_manager);

        void generateChunk(glm::vec2 pos);
};

#endif
