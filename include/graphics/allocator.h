#ifndef VULKAN_ALLOCATOR_H
#define VULKAN_ALLOCATOR_H

#include <cstdint>
#include <functional>
#include <optional>
#include <glm/glm.hpp>

#include "graphics/buffer.h"

class Device;
class BufferManager;

class Allocator {
public:
    Allocator() = default;
    Allocator(const Allocator&) = delete;
    Allocator& operator=(const Allocator&) = delete;
    Allocator(Allocator&&) noexcept = default;
    Allocator& operator=(Allocator&&) noexcept = default;
    Allocator(int p_flag, uint32_t p_nbBlock, uint32_t p_blockSize, std::reference_wrapper<Buffer> p_staging, Device& p_device);
    
    void alloc(const void* p_data, uint32_t p_size, uint32_t p_srcOffset, uint32_t p_dstOffset, BufferManager& p_buffer_manager);
    void extractData(void* p_dst, uint32_t p_nbBlock, uint32_t p_offset, BufferManager& p_buffer_manager);

    void cleanup();

    Buffer& getBuffer() { return _buffer; }
    const Buffer& getBuffer() const { return _buffer; }
    uint32_t getBlockSize() const { return _blockSize; }

private:
    Buffer _buffer;

    uint32_t _blockSize;
    VkDeviceSize _size;

    std::optional<std::reference_wrapper<Buffer>> _staging;
    Device* _device = nullptr;
};

#endif
