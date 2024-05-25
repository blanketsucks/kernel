#pragma once

#include <kernel/devices/block_device.h>

namespace kernel::devices {

class DiskDevice : public BlockDevice {
public:
    virtual ~DiskDevice() = default;

    virtual bool read_blocks(void* buffer, size_t count, size_t block) override = 0;
    virtual bool write_blocks(const void* buffer, size_t count, size_t block) override = 0;

protected:
    DiskDevice(u32 major, u32 minor, size_t block_size) : BlockDevice(major, minor, block_size) {}
};

}