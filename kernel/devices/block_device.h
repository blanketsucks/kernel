#pragma once

#include <kernel/common.h>
#include <kernel/devices/device.h>

namespace kernel {

class BlockDevice : public Device {
public:
    virtual ~BlockDevice() = default;

    ErrorOr<size_t> read(void* buffer, size_t size, size_t offset) override;
    ErrorOr<size_t> write(const void* buffer, size_t size, size_t offset) override;

    ErrorOr<bool> read_block(void* buffer, size_t block);
    ErrorOr<bool> write_block(const void* buffer, size_t block);

    size_t block_size() const { return m_block_size; }
    size_t max_addressable_block() const { return m_max_addressable_block; }

    // The maximum number of blocks that can be read/written in a single operation
    virtual size_t max_io_block_count() const = 0;

    virtual ErrorOr<bool> read_blocks(void* buffer, size_t count, size_t block) = 0;
    virtual ErrorOr<bool> write_blocks(const void* buffer, size_t count, size_t block) = 0;

    bool is_block_device() const final override { return true; }

protected:
    BlockDevice(DeviceMajor major, u32 minor, size_t block_size) : Device(major, minor), m_block_size(block_size) {}

    size_t m_block_size;
    size_t m_max_addressable_block = 0;
};

}