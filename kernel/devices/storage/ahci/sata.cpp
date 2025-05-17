#include <kernel/devices/storage/ahci/sata.h>

namespace kernel {

size_t SATADevice::max_io_block_count() const {
    return (AHCIPort::PRDT_BUFFER_SIZE * AHCIPort::MAX_PRDT_COUNT) / block_size();
}

ErrorOr<bool> SATADevice::read_blocks(void* buffer, size_t count, size_t block) {
    if (count > this->max_io_block_count()) {
        return Error(EINVAL);
    }

    TRY(m_port->read_sectors(block, count, reinterpret_cast<u8*>(buffer)));
    return true;
}

ErrorOr<bool> SATADevice::write_blocks(const void* buffer, size_t count, size_t block) {
    if (count > this->max_io_block_count()) {
        return Error(EINVAL);
    }

    TRY(m_port->write_sectors(block, count, reinterpret_cast<const u8*>(buffer)));
    return true;
}


}