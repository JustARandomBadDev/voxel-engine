#ifndef VOXEL_H
#define VOXEL_H

#include <cstdint>

struct Voxel {
    uint16_t id = 0;
    bool transparent = false;

    Voxel() = default;

    Voxel(uint16_t pid)
    : id(pid), transparent(false) {}
};

#endif
