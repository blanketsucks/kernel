#include <kernel/devices/storage/partition.h>

namespace kernel {

PartitionDevice* PartitionDevice::create(BlockDevice* device, size_t offset) {
    return new PartitionDevice(device, offset);
}

bool PartitionDevice::read_blocks(void* buffer, size_t count, size_t block) {
    return m_device->read_blocks(buffer, count, block + m_offset);
}

bool PartitionDevice::write_blocks(const void* buffer, size_t count, size_t block) {
    return m_device->write_blocks(buffer, count, block + m_offset);
}

}