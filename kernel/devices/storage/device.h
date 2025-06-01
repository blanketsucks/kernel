#pragma once

#include <kernel/devices/block_device.h>
#include <kernel/devices/storage/manager.h>
#include <kernel/devices/storage/partitions.h>

namespace kernel {

class StorageDevicePartition;

class StorageDevice : public BlockDevice {
public:
    enum Type {
        PATA,
        PATAPI,
        SATA,
        SATAPI,
    };

    virtual ~StorageDevice() = default;

    virtual ErrorOr<bool> read_blocks(void* buffer, size_t count, size_t block) override = 0;
    virtual ErrorOr<bool> write_blocks(const void* buffer, size_t count, size_t block) override = 0;

    virtual Type type() const = 0;

    RefPtr<StorageDevicePartition> partition(size_t index) const;
    size_t partitions() const { return m_partitions.size(); }

protected:
    StorageDevice(size_t block_size) : BlockDevice(DeviceMajor::Storage, StorageManager::generate_device_minor(), block_size) {}

private:
    friend class StorageManager;

    Vector<RefPtr<StorageDevicePartition>> m_partitions;
};

class StorageDevicePartition : public BlockDevice {
public:
    static RefPtr<StorageDevicePartition> create(StorageDevice* device, const PartitionEntry& partition);

    size_t max_io_block_count() const override;

    PartitionEntry const& partition() const { return m_partition; }
    StorageDevice* device() const { return m_device; }

    ErrorOr<bool> read_blocks(void* buffer, size_t count, size_t block) override;
    ErrorOr<bool> write_blocks(const void* buffer, size_t count, size_t block) override;

private:
    friend class Device;
    StorageDevicePartition(StorageDevice* device, const PartitionEntry& partition);

    StorageDevice* m_device;
    PartitionEntry m_partition;
};

}