#ifndef VOXEL_DATA_H
#define VOXEL_DATA_H

#include <vector>

struct VoxelData {
    int textures;
    float shininess;
    bool transparent;
};

extern std::vector<VoxelData> VOXEL_DATAS;

#endif