#include <kernel/devices/partition.h>

namespace kernel::devices {

PartitionDevice* PartitionDevice::create(u32 major, u32 minor, BlockDevice* device, size_t offset) {
    return new PartitionDevice(major, minor, device, offset);
}

bool PartitionDevice::read_blocks(void* buffer, size_t count, size_t block) {
    return m_device->read_blocks(buffer, count, block + m_offset);
}

bool PartitionDevice::write_blocks(const void* buffer, size_t count, size_t block) {
    return m_device->write_blocks(buffer, count, block + m_offset);
}

}