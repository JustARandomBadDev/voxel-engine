#ifndef VOXEL_H
#define VOXEL_H

#include <cstdint>

struct Voxel {
    uint16_t id = 0;

    Voxel() = default;

    Voxel(uint16_t pid)
    : id(pid) {}
};

#endif
