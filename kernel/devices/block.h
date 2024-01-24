#pragma once

#include <kernel/common.h>
#include <kernel/devices/device.h>

namespace kernel::devices {

class BlockDevice : public Device {
public:
    virtual ~BlockDevice() = default;

    size_t read(void* buffer, size_t size, size_t offset) override;
    size_t write(const void* buffer, size_t size, size_t offset) override;

    bool read_block(void* buffer, size_t block);
    bool write_block(const void* buffer, size_t block);

    size_t block_size() const { return m_block_size; }

    virtual bool read_blocks(void* buffer, size_t count, size_t block) = 0;
    virtual bool write_blocks(const void* buffer, size_t count, size_t block) = 0;

    bool is_block_device() const final override { return true; }

protected:
    BlockDevice(u32 major, u32 minor, size_t block_size) : Device(major, minor), m_block_size(block_size) {}

    size_t m_block_size;
};

}