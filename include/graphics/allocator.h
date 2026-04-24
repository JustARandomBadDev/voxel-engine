#ifndef VULKAN_ALLOCATOR_H
#define VULKAN_ALLOCATOR_H

#include <cstdint>
#include <limits>
#include <vector>

constexpr uint32_t INVALID_BUFFER_ID = std::numeric_limits<uint32_t>::max();

struct BufferAllocation {
    uint32_t bufferId = INVALID_BUFFER_ID;
    uint32_t offsetBlocks = 0;
    uint32_t reservedBlocks = 0;

    bool isValid() const {
        return bufferId != INVALID_BUFFER_ID && reservedBlocks > 0;
    }
};

class Allocator {
public:
    void init(uint32_t p_buffer_id, uint32_t p_capacity_blocks, uint32_t p_block_size);

    bool canAlloc(uint32_t p_reserved_blocks) const;
    BufferAllocation alloc(uint32_t p_reserved_blocks);
    void free(const BufferAllocation& p_allocation);
    void reset();

    uint32_t getBufferId() const { return _buffer_id; }
    uint32_t getBlockSize() const { return _block_size; }
    uint32_t getCommittedBlockCount() const { return _committed_blocks; }
    uint32_t getCapacityBlocks() const { return _capacity_blocks; }

private:
    struct FreeRange {
        uint32_t offsetBlocks = 0;
        uint32_t blockCount = 0;
    };

    uint32_t _buffer_id = INVALID_BUFFER_ID;
    uint32_t _capacity_blocks = 0;
    uint32_t _block_size = 0;
    uint32_t _committed_blocks = 0;
    std::vector<FreeRange> _free_ranges;

    int findFreeRange(uint32_t p_reserved_blocks) const;
};

#endif
