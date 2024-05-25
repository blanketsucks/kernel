#include <kernel/devices/block_device.h>
#include <std/string.h>

namespace kernel::devices {

ssize_t BlockDevice::read(void* buffer, size_t size, size_t offset) {
    size_t block = offset / this->block_size();
    size_t block_offset = offset % this->block_size();

    size_t bytes_read = 0;
    u8 block_buffer[this->block_size()];

    while (bytes_read < size) {
        this->read_block(block_buffer, block);

        size_t bytes_to_read = std::min(size - bytes_read, this->block_size() - block_offset);
        memcpy(reinterpret_cast<u8*>(buffer) + bytes_read, block_buffer + block_offset, bytes_to_read);

        bytes_read += bytes_to_read;
        block_offset = 0;

        block++;
    }

    return bytes_read;
}

ssize_t BlockDevice::write(const void* buffer, size_t size, size_t offset) {
    size_t block = offset / this->block_size();
    size_t block_offset = offset % this->block_size();

    size_t bytes_written = 0;
    u8 block_buffer[this->block_size()];

    while (bytes_written < size) {
        this->read_block(block_buffer, block);

        size_t bytes_to_write = std::min(size - bytes_written, this->block_size() - block_offset);
        memcpy(block_buffer + block_offset, reinterpret_cast<const u8*>(buffer) + bytes_written, bytes_to_write);

        this->write_block(block_buffer, block);

        bytes_written += bytes_to_write;
        block_offset = 0;

        block++;
    }

    return bytes_written;
}

bool BlockDevice::read_block(void* buffer, size_t block) {
    return this->read_blocks(buffer, 1, block);
}

bool BlockDevice::write_block(const void* buffer, size_t block) {
    return this->write_blocks(buffer, 1, block);
}

}