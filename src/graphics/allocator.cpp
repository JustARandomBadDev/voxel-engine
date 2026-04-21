#include "graphics/allocator.h"

#include <sstream>
#include <string.h>

#include "graphics/device.h"
#include "graphics/buffer_manager.h"

Allocator::Allocator(int p_usage, uint32_t p_nbBlock, uint32_t p_blockSize, std::reference_wrapper<Buffer> p_staging, Device& p_device)
: _blockSize(p_blockSize), _size(p_nbBlock*p_blockSize), _staging(std::make_optional(p_staging))
{
    _buffer.createBuffer(_size, p_usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, p_device);
    _device = &p_device;
}

void Allocator::alloc(const void* p_data, uint32_t p_nbBlock, uint32_t p_srcOffset, uint32_t p_dstOffset, BufferManager& p_buffer_manager) {
    if (!_staging.has_value() || _device == nullptr) {
        throw std::runtime_error("Allocator::alloc() -> allocator not initialized");
    }

    const VkDeviceSize size = static_cast<VkDeviceSize>(p_nbBlock) * _blockSize;
    const VkDeviceSize dstOffsetBytes = static_cast<VkDeviceSize>(p_dstOffset) * _blockSize;
    const VkDeviceSize stagingCapacity = _staging.value().get().getSize();

    if (dstOffsetBytes + size > _size) {
        std::ostringstream oss;
        oss << "Allocator::alloc() -> destination allocator overflow: "
            << "requested=" << size
            << " bytes, dstOffsetBytes=" << dstOffsetBytes
            << ", allocatorCapacity=" << _size
            << " bytes";
        throw std::runtime_error(oss.str());
    }

    if (p_srcOffset + size > stagingCapacity) {
        const VkDeviceSize remainingBytes = p_srcOffset <= stagingCapacity ? stagingCapacity - p_srcOffset : 0;
        std::ostringstream oss;
        oss << "Allocator::alloc() -> staging overflow: "
            << "requested=" << size
            << " bytes, srcOffset=" << p_srcOffset
            << " bytes, stagingCapacity=" << stagingCapacity
            << " bytes, remaining=" << remainingBytes
            << " bytes";
        throw std::runtime_error(oss.str());
    }

    void* data;
    
    vkMapMemory(_device->getDevice(), _staging.value().get().getBufferMemory(), p_srcOffset, size, 0, &data);
    memcpy(data, p_data, size);
    vkUnmapMemory(_device->getDevice(), _staging.value().get().getBufferMemory());

    p_buffer_manager.copyBuffer(_staging.value().get(), _buffer, size, p_srcOffset, p_dstOffset * _blockSize);
}

// Fonctionne plus (copyBuffer ne se fait plus directement, faut faire un applyCopies)
void Allocator::extractData(void* p_dst, uint32_t p_nbBlock, uint32_t p_offset, BufferManager& p_buffer_manager) {
    if (!_staging.has_value() || _device == nullptr) {
        throw std::runtime_error("Allocator::extractData() -> allocator not initialized");
    }

    uint32_t size = p_nbBlock * _blockSize;

    p_buffer_manager.copyBuffer(_buffer, _staging.value().get(), size, p_offset * _blockSize, 0);

    void* data;
    vkMapMemory(_device->getDevice(), _staging.value().get().getBufferMemory(), 0, size, 0, &data);
    memcpy(p_dst, data, size);
    vkUnmapMemory(_device->getDevice(), _staging.value().get().getBufferMemory());
}

void Allocator::cleanup() {
    _buffer.cleanup();
}
