#ifndef VULKAN_ALLOCATORS_MANAGER_H
#define VULKAN_ALLOCATORS_MANAGER_H

#include <cstdint>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <unordered_map>

#include "graphics/buffer.h"
#include "graphics/vertex.h"
#include "graphics/allocator.h"
#include "engine/mesh.h"

class BufferManager;
class Device;

const uint32_t NB_VERTEX_PER_BLOCK = 4;
const uint32_t NB_INDEX_PER_BLOCK = 6;

struct DrawIndirectCommand {
    uint32_t indexCount;    // Nombre d'indices à dessiner
    uint32_t instanceCount; // Toujours 1 pour du voxel
    uint32_t indexOffset;   // Offset dans l’index buffer
    uint32_t vertexOffset;  // Offset dans le vertex buffer
    uint32_t firstInstance; // Toujours 0 dans ce cas
};

struct AllocInfo {
    uint32_t dataBlock;
    uint32_t maxDataBlock;
    uint32_t indirectBlock;
};

class AllocatorManager {
public:
    void init(Device& p_device);
    int  allocMesh(Mesh& mesh, int pid, BufferManager& p_buffer_manager);
    void freeMesh(int pid);
    void resetStagingOffset() { _stagingOffset = 0; };

    void cleanup();

    Buffer& getVertexBuffer()   { return _vertexAllocator.getBuffer(); }
    Buffer& getIndexBuffer()    { return _indexAllocator.getBuffer(); }
    Buffer& getIndirectBuffer() { return _indirectAllocator.getBuffer(); }
    
    uint32_t     getVertexCount()    { return _nbDataBlock * NB_VERTEX_PER_BLOCK; }
    uint32_t     getIndexCount()     { return _nbDataBlock * NB_INDEX_PER_BLOCK; }
    uint32_t     getIndirectCount()  { return _nbIndirectBlock; }

private:
    Allocator _vertexAllocator;
    Allocator _indexAllocator;
    Allocator _indirectAllocator;
    Buffer _staging;

    uint32_t _nbDataBlock;
    uint32_t _nbIndirectBlock;

    uint32_t _id;

    uint32_t _stagingOffset;

    std::unordered_map<uint32_t, AllocInfo> _used;
    std::vector<AllocInfo> _freeList;
    std::vector<int> _freeId;

    int availableAlloc(uint32_t p_nbBlock);
    void newAlloc(uint32_t p_nbBlock, uint32_t p_maxNbBlock, AllocInfo& p_infos, int p_id);
    void freeIndirectBlock(uint32_t p_offset);
};

#endif
