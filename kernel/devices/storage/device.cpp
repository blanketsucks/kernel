#include <kernel/devices/storage/device.h>

namespace kernel {

RefPtr<StorageDevicePartition> StorageDevice::partition(size_t index) const {
    if (index >= m_partitions.size()) {
        return nullptr;
    }

    return m_partitions[index];
}

RefPtr<StorageDevicePartition> StorageDevicePartition::create(StorageDevice* device, const PartitionEntry& partition) {
    return RefPtr<StorageDevicePartition>(new StorageDevicePartition(device, partition));
}

StorageDevicePartition::StorageDevicePartition(StorageDevice* device, const PartitionEntry& partition)
    : BlockDevice(DeviceMajor::StoragePartition, StorageManager::generate_partition_minor(), device->block_size()),
    m_device(device), m_partition(partition) {
    m_max_addressable_block = partition.size;
}

size_t StorageDevicePartition::max_io_block_count() const {
    return m_device->max_io_block_count();
}

bool StorageDevicePartition::read_blocks(void* buffer, size_t count, size_t block) {
    return m_device->read_blocks(buffer, count, block + m_partition.offset);
}

bool StorageDevicePartition::write_blocks(const void* buffer, size_t count, size_t block) {
    return m_device->write_blocks(buffer, count, block + m_partition.offset);
}
    

}