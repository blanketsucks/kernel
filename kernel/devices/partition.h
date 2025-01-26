#pragma once

#include <kernel/common.h>
#include <kernel/devices/block_device.h>

namespace kernel::devices {

class PartitionDevice : public BlockDevice {
public:
    static PartitionDevice* create(u32 major, u32 minor, BlockDevice* device, size_t offset);

    BlockDevice* device() const { return m_device; }
    size_t offset() const { return m_offset; }

    size_t max_io_block_count() const override { return m_device->max_io_block_count(); }

    bool read_blocks(void* buffer, size_t count, size_t block) override;
    bool write_blocks(const void* buffer, size_t count, size_t block) override;

private:
    PartitionDevice(
        u32 major, u32 minor, BlockDevice* device, size_t offset
    ) : BlockDevice(major, minor, device->block_size()), m_device(device), m_offset(offset) {}

    BlockDevice* m_device;
    size_t m_offset;
};

}