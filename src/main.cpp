#include <glm/glm.hpp>

#include <chrono>
#include <cmath>
#include <iostream>
#include <string>

#include "engine/voxel_engine.h"
#include "world/perlin_noise_2d.h"
#include "world/procedural_generator.h"

void procedural(VoxelEngine& engine) {
    int size = 27;
    PerlinNoise2D perlin_noise;

    for (int x = -(size/2); x < size/2; x++) {
        for (int y = -(size/2); y < size/2; y++) {
            glm::ivec2 pos = {x, y};

            float frequencies[PROCEDURAL_OCTAVES];
            float amplitudes[PROCEDURAL_OCTAVES];

            for (int i = 0; i < PROCEDURAL_OCTAVES; i++) {
                frequencies[i] = PROCEDURAL_FREQUENCY * static_cast<float>(std::pow(PROCEDURAL_MULT_FREQUENCY, i));
                amplitudes[i] = PROCEDURAL_AMPLITUDE * static_cast<float>(std::pow(PROCEDURAL_PERSISTENCE, i));
            }

            for (int vx = 0; vx < CHUNK_SIZE; vx++) {
                for (int vz = 0; vz < CHUNK_SIZE; vz++) {
                    float total = 0.0f;

                    for (int i = 0; i < PROCEDURAL_OCTAVES; i++) {
                        float fx = (pos.x * CHUNK_SIZE + vx) * frequencies[i];
                        float fz = (pos.y * CHUNK_SIZE + vz) * frequencies[i];
                        total += perlin_noise.noise_2d(fx, fz) * amplitudes[i];
                    }

                    int height = static_cast<int>(total);

                    int voxel_start = 0;
                    int voxel_end = 0;

                    if (height <= 50) {
                        voxel_end = height - 5;
                        for (int vy = voxel_start; vy < voxel_end; ++vy) {
                            engine.setVoxel({pos.x, vy / CHUNK_SIZE, pos.y}, {vx, vy % CHUNK_SIZE, vz}, 3);
                        }

                        voxel_start = voxel_end;
                        voxel_end = height + 1;
                        for (int vy = voxel_start; vy < voxel_end; ++vy) {
                            engine.setVoxel({pos.x, vy / CHUNK_SIZE, pos.y}, {vx, vy % CHUNK_SIZE, vz}, 9);
                        }

                        voxel_start = voxel_end;
                        voxel_end = 48;
                        for (int vy = voxel_start; vy < voxel_end; ++vy) {
                            engine.setVoxel({pos.x, vy / CHUNK_SIZE, pos.y}, {vx, vy % CHUNK_SIZE, vz}, 8);
                        }
                    } else {
                        voxel_end = height - 2;
                        for (int vy = voxel_start; vy < voxel_end; ++vy) {
                            engine.setVoxel({pos.x, vy / CHUNK_SIZE, pos.y}, {vx, vy % CHUNK_SIZE, vz}, 3);
                        }

                        voxel_start = voxel_end;
                        voxel_end = height;
                        for (int vy = voxel_start; vy < voxel_end; ++vy) {
                            engine.setVoxel({pos.x, vy / CHUNK_SIZE, pos.y}, {vx, vy % CHUNK_SIZE, vz}, 2);
                        }

                        engine.setVoxel({pos.x, height / CHUNK_SIZE, pos.y}, {vx, height % CHUNK_SIZE, vz}, 1);
                    }
                }
            }
        }
    }
}

template<typename Func>
double timeOf(Func func, std::string msg) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> elapsed = end - start;
    
    std::cout << msg << elapsed.count() << " ms" << std::endl;

    return elapsed.count();
}

void run() {
    VoxelEngine engine;

    engine.init({0, 10.0f, 0}, 70);

    // ------------------------ Temps Allocation ------------------------ //

    double total = 0;

    total += timeOf([&]() { procedural(engine); }, "Temps de génération du monde : ");
    total += timeOf([&]() { engine.update(); }, "Temps de mise à jour moteur : ");

    std::cout << "Temps total : " << total << std::endl;

    // ------------------------------------------------------------------ //

    auto lastTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    double elapsedTime;
    int frameCount = 0;

    while (engine.isRun()) {
        currentTime = std::chrono::high_resolution_clock::now();    
        elapsedTime = std::chrono::duration<double, std::milli>(currentTime - lastTime).count();
        frameCount++;
    
        if (elapsedTime >= 1000.0) {
            std::cout << "\rFPS: " << frameCount << " " << std::flush;
            frameCount = 0;
            lastTime = currentTime;
        }

        engine.update();
        engine.render();
    }

    engine.shutdown();
}

int main(int argc, char const *argv[]) {
    try {
        run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
