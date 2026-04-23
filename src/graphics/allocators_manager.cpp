#include "graphics/allocators_manager.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "graphics/buffer_manager.h"
#include "graphics/buffer.h"

namespace {

constexpr VkBufferUsageFlags kVertexBufferUsage =
    VK_BUFFER_USAGE_TRANSFER_DST_BIT |
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

constexpr VkBufferUsageFlags kIndexBufferUsage =
    VK_BUFFER_USAGE_TRANSFER_DST_BIT |
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
    VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
    VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

constexpr VkBufferUsageFlags kIndirectBufferUsage =
    VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
    VK_BUFFER_USAGE_TRANSFER_DST_BIT |
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

DrawIndirectCommand makeZeroIndirectCommand() {
    return DrawIndirectCommand{
        0,
        0,
        0,
        0,
        0
    };
}

} // namespace

void AllocatorManager::init(BufferManager& p_buffer_manager, const GpuAllocatorConfig& p_config) {
    if (p_config.meshDataBlockCapacityPerAllocator == 0 ||
        p_config.indirectCommandCapacityPerAllocator == 0 ||
        p_config.allocationMarginBlocks == 0) {
        throw std::runtime_error("AllocatorManager::init() -> invalid GPU allocator config");
    }

    cleanup();

    _mesh_capacity_blocks = p_config.meshDataBlockCapacityPerAllocator;
    _indirect_capacity_blocks = p_config.indirectCommandCapacityPerAllocator;
    _allocation_margin_blocks = p_config.allocationMarginBlocks;

    createPage(p_buffer_manager);
}

uint32_t AllocatorManager::createPage(BufferManager& p_buffer_manager) {
    const uint32_t vertexBufferId = p_buffer_manager.createManagedBuffer(
        static_cast<VkDeviceSize>(_mesh_capacity_blocks) * NB_VERTEX_PER_BLOCK * sizeof(Vertex),
        kVertexBufferUsage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    const uint32_t indexBufferId = p_buffer_manager.createManagedBuffer(
        static_cast<VkDeviceSize>(_mesh_capacity_blocks) * NB_INDEX_PER_BLOCK * sizeof(uint32_t),
        kIndexBufferUsage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    const uint32_t indirectBufferId = p_buffer_manager.createManagedBuffer(
        static_cast<VkDeviceSize>(_indirect_capacity_blocks) * sizeof(DrawIndirectCommand),
        kIndirectBufferUsage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    AllocationPage page;
    page.vertexAllocator.init(vertexBufferId, _mesh_capacity_blocks, static_cast<uint32_t>(NB_VERTEX_PER_BLOCK * sizeof(Vertex)));
    page.indexAllocator.init(indexBufferId, _mesh_capacity_blocks, static_cast<uint32_t>(NB_INDEX_PER_BLOCK * sizeof(uint32_t)));
    page.indirectAllocator.init(indirectBufferId, _indirect_capacity_blocks, static_cast<uint32_t>(sizeof(DrawIndirectCommand)));

    _pages.push_back(std::move(page));
    return static_cast<uint32_t>(_pages.size() - 1);
}

uint32_t AllocatorManager::findOrCreatePage(uint32_t p_data_reserved_blocks, BufferManager& p_buffer_manager) {
    for (size_t i = 0; i < _pages.size(); ++i) {
        AllocationPage& page = _pages[i];
        if (page.vertexAllocator.canAlloc(p_data_reserved_blocks) &&
            page.indexAllocator.canAlloc(p_data_reserved_blocks) &&
            page.indirectAllocator.canAlloc(1)) {
            return static_cast<uint32_t>(i);
        }
    }

    return createPage(p_buffer_manager);
}

void AllocatorManager::queueMeshUpload(
    Mesh& p_mesh,
    const MeshAllocInfo& p_infos,
    BufferManager& p_buffer_manager
) {
    const uint32_t dataBlocks = static_cast<uint32_t>(p_mesh.getVertex().size() / NB_VERTEX_PER_BLOCK);
    const AllocationPage& page = _pages[p_infos.pageIndex];

    DrawIndirectCommand indirectCommand{};
    indirectCommand.indexCount = static_cast<uint32_t>(p_mesh.getIndex().size());
    indirectCommand.instanceCount = 1;
    indirectCommand.indexOffset = p_infos.index.offsetBlocks * NB_INDEX_PER_BLOCK;
    indirectCommand.vertexOffset = p_infos.vertex.offsetBlocks * NB_VERTEX_PER_BLOCK;
    indirectCommand.firstInstance = 0;

    p_buffer_manager.enqueueUpload(
        page.vertexAllocator.getBufferId(),
        static_cast<VkDeviceSize>(p_infos.vertex.offsetBlocks) * page.vertexAllocator.getBlockSize(),
        p_mesh.getVertex().data(),
        static_cast<VkDeviceSize>(dataBlocks) * page.vertexAllocator.getBlockSize()
    );

    p_buffer_manager.enqueueUpload(
        page.indexAllocator.getBufferId(),
        static_cast<VkDeviceSize>(p_infos.index.offsetBlocks) * page.indexAllocator.getBlockSize(),
        p_mesh.getIndex().data(),
        static_cast<VkDeviceSize>(dataBlocks) * page.indexAllocator.getBlockSize()
    );

    p_buffer_manager.enqueueUpload(
        page.indirectAllocator.getBufferId(),
        static_cast<VkDeviceSize>(p_infos.indirect.offsetBlocks) * page.indirectAllocator.getBlockSize(),
        &indirectCommand,
        static_cast<VkDeviceSize>(page.indirectAllocator.getBlockSize())
    );
}

void AllocatorManager::queueZeroIndirect(const MeshAllocInfo& p_infos, BufferManager& p_buffer_manager) {
    const AllocationPage& page = _pages[p_infos.pageIndex];
    const DrawIndirectCommand zeroCommand = makeZeroIndirectCommand();

    p_buffer_manager.enqueueUpload(
        page.indirectAllocator.getBufferId(),
        static_cast<VkDeviceSize>(p_infos.indirect.offsetBlocks) * page.indirectAllocator.getBlockSize(),
        &zeroCommand,
        static_cast<VkDeviceSize>(page.indirectAllocator.getBlockSize())
    );
}

int AllocatorManager::allocMesh(Mesh& p_mesh, int p_pid, BufferManager& p_buffer_manager) {
    const uint32_t dataBlocks = static_cast<uint32_t>(p_mesh.getVertex().size() / NB_VERTEX_PER_BLOCK);
    const uint32_t reservedDataBlocks = std::max(1u, dataBlocks * _allocation_margin_blocks);

    if (reservedDataBlocks > _mesh_capacity_blocks) {
        std::ostringstream oss;
        oss << "AllocatorManager::allocMesh() -> mesh allocation exceeds per-buffer capacity: "
            << "requestedBlocks=" << reservedDataBlocks
            << ", pageCapacityBlocks=" << _mesh_capacity_blocks;
        throw std::runtime_error(oss.str());
    }

    if (_indirect_capacity_blocks == 0) {
        throw std::runtime_error("AllocatorManager::allocMesh() -> indirect allocator capacity is 0");
    }

    int out = p_pid;
    if (out == -1) {
        if (_freeId.empty()) {
            out = static_cast<int>(_next_id++);
        } else {
            out = _freeId.back();
            _freeId.pop_back();
        }
    } else if (out < 0) {
        std::ostringstream oss;
        oss << "AllocatorManager::allocMesh() -> invalid allocation id: " << out;
        throw std::runtime_error(oss.str());
    }

    MeshAllocInfo infos;
    bool needsFreshAllocation = true;

    const auto usedIt = _used.find(static_cast<uint32_t>(out));
    if (usedIt != _used.end()) {
        infos = usedIt->second;
        if (dataBlocks <= infos.vertex.reservedBlocks &&
            dataBlocks <= infos.index.reservedBlocks &&
            infos.indirect.reservedBlocks >= 1) {
            needsFreshAllocation = false;
        } else {
            AllocationPage& oldPage = _pages[infos.pageIndex];
            oldPage.vertexAllocator.free(infos.vertex);
            oldPage.indexAllocator.free(infos.index);
            oldPage.indirectAllocator.free(infos.indirect);
            queueZeroIndirect(infos, p_buffer_manager);
        }
    }

    if (needsFreshAllocation) {
        const uint32_t pageIndex = findOrCreatePage(reservedDataBlocks, p_buffer_manager);
        AllocationPage& page = _pages[pageIndex];

        infos.pageIndex = pageIndex;
        infos.vertex = page.vertexAllocator.alloc(reservedDataBlocks);
        infos.index = page.indexAllocator.alloc(reservedDataBlocks);
        infos.indirect = page.indirectAllocator.alloc(1);
    }

    _used[static_cast<uint32_t>(out)] = infos;
    queueMeshUpload(p_mesh, infos, p_buffer_manager);

    return out;
}

void AllocatorManager::freeMesh(int p_pid, BufferManager& p_buffer_manager) {
    if (p_pid < 0) {
        std::ostringstream oss;
        oss << "AllocatorManager::freeMesh() -> invalid allocation id: " << p_pid;
        throw std::runtime_error(oss.str());
    }

    const auto usedIt = _used.find(static_cast<uint32_t>(p_pid));
    if (usedIt == _used.end()) {
        std::ostringstream oss;
        oss << "AllocatorManager::freeMesh() -> unknown or already freed allocation id: " << p_pid;
        throw std::runtime_error(oss.str());
    }

    const MeshAllocInfo infos = usedIt->second;
    AllocationPage& page = _pages[infos.pageIndex];
    page.vertexAllocator.free(infos.vertex);
    page.indexAllocator.free(infos.index);
    page.indirectAllocator.free(infos.indirect);
    queueZeroIndirect(infos, p_buffer_manager);

    _used.erase(usedIt);
    _freeId.push_back(p_pid);
}

uint32_t AllocatorManager::getIndirectCount() const {
    uint32_t total = 0;
    for (const AllocationPage& page : _pages) {
        total += page.indirectAllocator.getCommittedBlockCount();
    }
    return total;
}

void AllocatorManager::cleanup() {
    _pages.clear();
    _used.clear();
    _freeId.clear();
    _next_id = 0;
    _mesh_capacity_blocks = 0;
    _indirect_capacity_blocks = 0;
    _allocation_margin_blocks = 1;
}
