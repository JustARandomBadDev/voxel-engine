#include "graphics/allocators_manager.h"

#include <iostream>
#include <string.h>
#include <vulkan/vulkan.h>

#include "graphics/device.h"
#include "graphics/buffer_manager.h"
#include "engine/mesh.h"

void AllocatorManager::init(Device& p_device, const GpuAllocatorConfig& p_config) {
    if (p_config.meshDataBlockCapacityPerAllocator == 0 ||
        p_config.indirectCommandCapacityPerAllocator == 0 ||
        p_config.stagingBufferBytes == 0 ||
        p_config.allocationMarginBlocks == 0) {
        throw std::runtime_error("AllocatorManager::init() -> invalid GPU allocator config");
    }

    _staging.createBuffer(
        p_config.stagingBufferBytes,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        p_device
    );

    _vertexAllocator = Allocator(
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        p_config.meshDataBlockCapacityPerAllocator,
        static_cast<uint32_t>(NB_VERTEX_PER_BLOCK * sizeof(Vertex)),
        _staging,
        p_device
    );

    _indexAllocator = Allocator(
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        p_config.meshDataBlockCapacityPerAllocator,
        static_cast<uint32_t>(NB_INDEX_PER_BLOCK * sizeof(uint32_t)),
        _staging,
        p_device
    );

    _indirectAllocator = Allocator(
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        p_config.indirectCommandCapacityPerAllocator,
        static_cast<uint32_t>(sizeof(DrawIndirectCommand)),
        _staging,
        p_device
    );

    _id = 0;
    _nbDataBlock = 0;
    _nbIndirectBlock = 0;
    _stagingOffset = 0;
    _allocationMarginBlocks = p_config.allocationMarginBlocks;
}

int AllocatorManager::allocMesh(Mesh& p_mesh, int p_pid, BufferManager& p_buffer_manager) {
    auto& vertex = p_mesh.getVertex();
    auto& index = p_mesh.getIndex();
    uint32_t nbBlock = static_cast<uint32_t>(vertex.size()) / NB_VERTEX_PER_BLOCK;
    uint32_t maxNbBlock = nbBlock * _allocationMarginBlocks;
    
    DrawIndirectCommand indirectCommand;
    indirectCommand.indexCount = index.size();
    indirectCommand.instanceCount = 1;
    indirectCommand.firstInstance = 0;

    AllocInfo infos;

    int out = p_pid;
    if (out == -1) {
        if (_freeId.empty()) {
            out = _id++;
        } else {
            out = _freeId[0];
            _freeId.erase(_freeId.begin());
        }
        newAlloc(nbBlock, maxNbBlock, infos, out);
    } else {
        infos = _used[out];
        if (nbBlock > infos.maxDataBlock) {
            _used.erase(out);
            _freeList.push_back(infos);
            freeIndirectBlock(infos.indirectBlock);
            newAlloc(nbBlock, maxNbBlock, infos, out);
        }
    }

    indirectCommand.vertexOffset = static_cast<uint32_t>(infos.dataBlock * NB_VERTEX_PER_BLOCK);
    indirectCommand.indexOffset = static_cast<uint32_t>(infos.dataBlock * NB_INDEX_PER_BLOCK);

    auto tryAlloc = [&](Allocator& p_allocator, void* p_data, uint32_t p_blocks, uint32_t& p_stagingOffset, uint32_t p_dest_offset) {
        uint32_t size = p_blocks * p_allocator.getBlockSize();

        if (p_stagingOffset + size > _staging.getSize()) {
            p_buffer_manager.applyCopies();
            p_stagingOffset = 0;
        }
        p_allocator.alloc(p_data, p_blocks, p_stagingOffset, p_dest_offset);
        p_stagingOffset += size;
    };

    tryAlloc(_vertexAllocator, vertex.data(), nbBlock, _stagingOffset, infos.dataBlock);
    tryAlloc(_indexAllocator, index.data(), nbBlock, _stagingOffset, infos.dataBlock);
    tryAlloc(_indirectAllocator, &indirectCommand, 1, _stagingOffset, infos.indirectBlock);

    return out;
}

void AllocatorManager::freeMesh(int p_pid) {
    AllocInfo infos = _used[p_pid];

    _freeList.push_back(infos);

    freeIndirectBlock(infos.indirectBlock);

    _used.erase(p_pid);

    _freeId.push_back(p_pid);
}

int AllocatorManager::availableAlloc(uint32_t p_nbBlock) {
    if (_freeList.empty()) return -1;

    uint32_t index = 0;
    for (AllocInfo info : _freeList) {
        if (p_nbBlock * _allocationMarginBlocks < info.maxDataBlock) return index;

        index++;
    }

    return -1;
}

void AllocatorManager::newAlloc(uint32_t p_nbBlock, uint32_t p_maxNbBlock, AllocInfo& p_infos, int p_pid) {
    int allocBlockId = availableAlloc(p_nbBlock);

    if (allocBlockId < 0) {
        // Dans le cas ou la freelist ne contient pas de block valide

        p_infos = {
            static_cast<uint32_t>(_nbDataBlock),
            static_cast<uint32_t>(p_maxNbBlock),
            static_cast<uint32_t>(_nbIndirectBlock)
        };
    
        _used[p_pid] = p_infos;
        
        _nbDataBlock += p_maxNbBlock;
        _nbIndirectBlock++;
    } else {
        // Dans le cas ou la freelist contient un block valide pour le chunk

        p_infos = _freeList[allocBlockId];
        _freeList.erase(_freeList.begin()+allocBlockId);

        _used[p_pid] = p_infos;
    }
}

void AllocatorManager::freeIndirectBlock(uint32_t p_offset) {
    DrawIndirectCommand indirectCommand;
    indirectCommand.indexCount = 0;
    indirectCommand.instanceCount = 0;
    indirectCommand.indexOffset = 0;
    indirectCommand.vertexOffset = 0;
    indirectCommand.firstInstance = 0;

    _indirectAllocator.alloc(&indirectCommand, 1, _stagingOffset, p_offset);

    _stagingOffset += _indirectAllocator.getBlockSize();
}

void AllocatorManager::cleanup() {
    _vertexAllocator.cleanup();
    _indexAllocator.cleanup();
    _indirectAllocator.cleanup();
    _staging.cleanup();
}
