#include <kernel/devices/block_device.h>

#include <std/string.h>
#include <std/heap_array.h>

namespace kernel {

ErrorOr<size_t> BlockDevice::read(void* buffer, size_t size, size_t offset) {
    size_t block_size = this->block_size();

    size_t block = offset / block_size;
    size_t block_offset = offset % block_size;

    size_t total_blocks = size / block_size;
    size_t remaining_bytes = size % block_size;

    u8* buf = reinterpret_cast<u8*>(buffer);
    std::HeapArray<u8> block_buffer { block_size };

    if (block_offset) {
        TRY(this->read_block(block_buffer.data(), block));
        memcpy(buf, block_buffer.data() + block_offset, block_size - block_offset);

        buf += block_size - block_offset;
        block++;
        total_blocks--;
    }

    TRY(this->read_blocks(buf, total_blocks, block));
    if (remaining_bytes) {
        TRY(this->read_block(block_buffer.data(), block + total_blocks));
        memcpy(buf + size - remaining_bytes, block_buffer.data(), remaining_bytes);
    }

    return size;
}

ErrorOr<size_t> BlockDevice::write(const void* buffer, size_t size, size_t offset) {
    size_t block = offset / this->block_size();
    size_t block_offset = offset % this->block_size();

    size_t nbytes = 0;
    std::HeapArray<u8> block_buffer { block_size() };

    while (nbytes < size) {
        TRY(this->read_block(block_buffer.data(), block));

        size_t bytes_to_write = std::min(size - nbytes, this->block_size() - block_offset);
        memcpy(block_buffer.data() + block_offset, reinterpret_cast<const u8*>(buffer) + nbytes, bytes_to_write);

        TRY(this->write_block(block_buffer.data(), block));

        nbytes += bytes_to_write;
        block_offset = 0;

        block++;
    }

    return nbytes;
}

ErrorOr<bool> BlockDevice::read_block(void* buffer, size_t block) {
    return this->read_blocks(buffer, 1, block);
}

ErrorOr<bool> BlockDevice::write_block(const void* buffer, size_t block) {
    return this->write_blocks(buffer, 1, block);
}

}
