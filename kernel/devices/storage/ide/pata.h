#pragma once

#include <kernel/process/blocker.h>
#include <kernel/devices/storage/device.h>
#include <kernel/devices/storage/ata.h>
#include <kernel/arch/irq.h>
#include <kernel/pci/pci.h>
#include <kernel/arch/io.h>

namespace kernel {

class PATADevice : public StorageDevice, IRQHandler {
public:
    struct PhysicalRegionDescriptor {
        u32 base;
        u16 size;
        u16 flags;
    } PACKED;

    constexpr static size_t DMA_BUFFER_SIZE = PAGE_SIZE * 10;

    static RefPtr<PATADevice> create(ata::Channel, ata::Drive, pci::Address);

    ata::Channel channel() const { return m_channel; }
    ata::Drive drive() const { return m_drive; }
    
    Type type() const override { return m_type; }

    bool has_48bit_pio() const { return m_has_48bit_pio; }

    size_t max_io_block_count() const override;

    io::Port control_port() const { return m_control; }
    io::Port data_port() const { return m_data; }
    io::Port bus_master_port() const { return m_bus_master; }

    void wait_while_busy() const;
    void wait_for_irq();
    void poll() const; // An alternative to IRQs

    void prepare_for(ata::Command command, size_t lba, u16 sectors);

    bool read_blocks(void* buffer, size_t count, size_t block) override;
    bool write_blocks(const void* buffer, size_t count, size_t block) override;

    void read_sectors(size_t lba, u8 sectors, u8* buffer);
    void write_sectors(size_t lba, u8 sectors, const u8* buffer);

    void read_sectors_with_dma(size_t lba, u8 sectors, u8* buffer);
    void write_sectors_with_dma(size_t lba, u8 sectors, const u8* buffer);
    
private:
    PATADevice(ata::Channel channel, ata::Drive drive, pci::Address address);

    void handle_irq() override;

    ata::Channel m_channel;
    ata::Drive m_drive;

    Type m_type;

    io::Port m_control;
    io::Port m_data;
    io::Port m_bus_master;

    BooleanBlocker m_irq_blocker;

    bool m_has_48bit_pio;
    bool m_has_dma;

    PhysicalRegionDescriptor* m_prdt = nullptr;
    u8* m_dma_buffer = nullptr;
};

}