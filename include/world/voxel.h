#ifndef VOXEL_H
#define VOXEL_H

#include <cstdint>

// id == 0 represents empty space. Non-zero ids refer to engine-side voxel/material data tables.
struct Voxel {
    uint16_t id = 0;

    Voxel() = default;

    Voxel(uint16_t pid)
    : id(pid) {}
};

#endif
