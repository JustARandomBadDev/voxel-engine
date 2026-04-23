#ifndef VULKAN_ALLOCATORS_MANAGER_H
#define VULKAN_ALLOCATORS_MANAGER_H

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "engine/mesh.h"
#include "engine/voxel_engine_config.h"
#include "graphics/allocator.h"

class BufferManager;

constexpr uint32_t NB_VERTEX_PER_BLOCK = 4;
constexpr uint32_t NB_INDEX_PER_BLOCK = 6;

struct DrawIndirectCommand {
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t indexOffset;
    uint32_t vertexOffset;
    uint32_t firstInstance;
};

struct AllocationPage {
    Allocator vertexAllocator;
    Allocator indexAllocator;
    Allocator indirectAllocator;
};

struct MeshAllocInfo {
    uint32_t pageIndex = 0;
    BufferAllocation vertex;
    BufferAllocation index;
    BufferAllocation indirect;
};

class AllocatorManager {
public:
    void init(BufferManager& p_buffer_manager, const GpuAllocatorConfig& p_config);
    int  allocMesh(Mesh& mesh, int pid, BufferManager& p_buffer_manager);
    void freeMesh(int pid, BufferManager& p_buffer_manager);
    void cleanup();

    const std::vector<AllocationPage>& getPages() const { return _pages; }
    uint32_t getIndirectCount() const;

private:
    std::vector<AllocationPage> _pages;
    std::unordered_map<uint32_t, MeshAllocInfo> _used;
    std::vector<int> _freeId;

    uint32_t _next_id = 0;
    uint32_t _mesh_capacity_blocks = 0;
    uint32_t _indirect_capacity_blocks = 0;
    uint32_t _allocation_margin_blocks = 1;

    uint32_t createPage(BufferManager& p_buffer_manager);
    uint32_t findOrCreatePage(uint32_t p_data_reserved_blocks, BufferManager& p_buffer_manager);
    void queueMeshUpload(
        Mesh& p_mesh,
        const MeshAllocInfo& p_infos,
        BufferManager& p_buffer_manager
    );
    void queueZeroIndirect(const MeshAllocInfo& p_infos, BufferManager& p_buffer_manager);
};

#endif
