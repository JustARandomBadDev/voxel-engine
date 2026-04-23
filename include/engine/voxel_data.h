#ifndef VOXEL_DATA_H
#define VOXEL_DATA_H

#include <vector>

// transparent controls both which CPU mesh stream a voxel uses and how adjacent face visibility is evaluated.
struct VoxelData {
    float shininess;
    bool transparent;
};

// Non-zero voxel ids use id - 1 to index this table.
extern std::vector<VoxelData> VOXEL_DATAS;

#endif
