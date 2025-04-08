#pragma once

#include <kernel/devices/storage/ahci/ports.h>
#include <kernel/devices/storage/device.h>

namespace kernel {

class SATADevice : public StorageDevice {
public:
    static RefPtr<SATADevice> create(AHCIPort* port, size_t max_addressable_block) {
        return RefPtr<SATADevice>(new SATADevice(port, max_addressable_block));
    }

    size_t max_io_block_count() const override;

    bool read_blocks(void* buffer, size_t count, size_t block) override;
    bool write_blocks(const void* buffer, size_t count, size_t block) override;

    Type type() const override { return SATA; }

private:
    SATADevice(AHCIPort* port, size_t max_addressable_block) : StorageDevice(SECTOR_SIZE), m_port(port) {
        m_max_addressable_block = max_addressable_block;
    }

    AHCIPort* m_port;
};

}