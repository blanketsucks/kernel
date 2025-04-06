#pragma once

#include <kernel/devices/block_device.h>
#include <kernel/devices/storage/manager.h>

namespace kernel {

class StorageDevice : public BlockDevice {
public:
    virtual ~StorageDevice() = default;

    virtual bool read_blocks(void* buffer, size_t count, size_t block) override = 0;
    virtual bool write_blocks(const void* buffer, size_t count, size_t block) override = 0;

protected:
    StorageDevice(size_t block_size) : BlockDevice(DeviceMajor::Storage, StorageManager::generate_device_minor(), block_size) {}
};

}