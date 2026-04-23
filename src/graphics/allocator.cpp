#include "graphics/allocator.h"

#include <sstream>
#include <stdexcept>

void Allocator::init(uint32_t p_buffer_id, uint32_t p_capacity_blocks, uint32_t p_block_size) {
    if (p_buffer_id == INVALID_BUFFER_ID) {
        throw std::runtime_error("Allocator::init() -> invalid buffer id");
    }

    if (p_capacity_blocks == 0 || p_block_size == 0) {
        throw std::runtime_error("Allocator::init() -> capacity and block size must be greater than 0");
    }

    _buffer_id = p_buffer_id;
    _capacity_blocks = p_capacity_blocks;
    _block_size = p_block_size;
    _committed_blocks = 0;
    _free_ranges.clear();
}

int Allocator::findFreeRange(uint32_t p_reserved_blocks) const {
    for (size_t i = 0; i < _free_ranges.size(); ++i) {
        if (_free_ranges[i].blockCount >= p_reserved_blocks) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

bool Allocator::canAlloc(uint32_t p_reserved_blocks) const {
    if (p_reserved_blocks == 0) return true;
    if (findFreeRange(p_reserved_blocks) >= 0) return true;
    return _committed_blocks + p_reserved_blocks <= _capacity_blocks;
}

BufferAllocation Allocator::alloc(uint32_t p_reserved_blocks) {
    if (p_reserved_blocks == 0) {
        throw std::runtime_error("Allocator::alloc() -> reserved block count must be greater than 0");
    }

    const int free_range_index = findFreeRange(p_reserved_blocks);
    if (free_range_index >= 0) {
        FreeRange& free_range = _free_ranges[free_range_index];
        BufferAllocation allocation {
            _buffer_id,
            free_range.offsetBlocks,
            p_reserved_blocks
        };

        free_range.offsetBlocks += p_reserved_blocks;
        free_range.blockCount -= p_reserved_blocks;

        if (free_range.blockCount == 0) {
            _free_ranges.erase(_free_ranges.begin() + free_range_index);
        }

        return allocation;
    }

    if (_committed_blocks + p_reserved_blocks > _capacity_blocks) {
        std::ostringstream oss;
        oss << "Allocator::alloc() -> allocator capacity exceeded: "
            << "requestedBlocks=" << p_reserved_blocks
            << ", committedBlocks=" << _committed_blocks
            << ", capacityBlocks=" << _capacity_blocks;
        throw std::runtime_error(oss.str());
    }

    BufferAllocation allocation {
        _buffer_id,
        _committed_blocks,
        p_reserved_blocks
    };

    _committed_blocks += p_reserved_blocks;
    return allocation;
}

void Allocator::free(const BufferAllocation& p_allocation) {
    if (!p_allocation.isValid()) return;

    if (p_allocation.bufferId != _buffer_id) {
        throw std::runtime_error("Allocator::free() -> allocation does not belong to this allocator");
    }

    _free_ranges.push_back({
        p_allocation.offsetBlocks,
        p_allocation.reservedBlocks
    });
}

void Allocator::reset() {
    _buffer_id = INVALID_BUFFER_ID;
    _capacity_blocks = 0;
    _block_size = 0;
    _committed_blocks = 0;
    _free_ranges.clear();
}
