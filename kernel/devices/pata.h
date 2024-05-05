#pragma once

#include <kernel/pci.h>
#include <kernel/arch/pic.h>
#include <kernel/devices/disk.h>
#include <kernel/process/blocker.h>

#include <std/enums.h>

namespace kernel::devices {

enum class ATACommand : u8 {
    Identify = 0xEC,
    Read = 0x20,    // For 28-bit PIO mode
    Write = 0x30,   // For 28-bit PIO mode
    CacheFlush = 0xE7,

    ReadExt = 0x24, // For 48-bit PIO mode
    WriteExt = 0x34 // For 48-bit PIO mode
};

enum class ATAStatus : u8 {
    Error = 1 << 0,
    Index = 1 << 1,
    CorrectedData = 1 << 2,
    DataRequest = 1 << 3,
    DriveFault = 1 << 5,
    DataReady = 1 << 6,
    Busy = 1 << 7
};

MAKE_ENUM_BITWISE_OPS(ATAStatus);

enum class ATARegister : u8 {
    Data = 0x00,
    Error = 0x01,
    Features = 0x01,
    SectorCount = 0x02,
    LBA0 = 0x03,
    LBA1 = 0x04,
    LBA2 = 0x05,
    Drive = 0x06,
    Status = 0x07,
    Command = 0x07,
    Control = 0x0C,
    AltStatus = 0x0C,

    BusMasterStatus = 0x2
};

enum class ATADrive : u8 {
    Master,
    Slave
};

enum class ATAChannel : u8 {
    Primary,
    Secondary
};

struct PhysicalRegionDescriptor {
    u32 base;
    u16 size;
    u16 flags;
} PACKED; 

class PATADevice : public DiskDevice, IRQHandler {
public:
    constexpr static u16 PRIMARY_CONTROL_PORT = 0x3F6;
    constexpr static u16 SECONDARY_CONTROL_PORT = 0x376;

    constexpr static u16 PRIMARY_DATA_PORT = 0x1F0;
    constexpr static u16 SECONDARY_DATA_PORT = 0x170;

    constexpr static u16 PRIMARY_IRQ = 14;
    constexpr static u16 SECONDARY_IRQ = 15;
    
    static PATADevice* create(ATAChannel channel, ATADrive drive);

    ATAChannel channel() const { return m_channel; }
    ATADrive drive() const { return m_drive; }

    u16 cylinders() const { return m_cylinders; }
    u16 heads() const { return m_heads; }
    u16 sectors_per_track() const { return m_sectors_per_track; }
    
    size_t sectors() const { return m_cylinders * m_heads * m_sectors_per_track; }

    bool has_48bit_pio() const { return m_has_48bit_pio; }

    size_t max_io_block_count() const; // The maximum number of blocks that can be read/written in a single operation

    u16 control_port() const { return m_control_port; }
    u16 data_port() const { return m_data_port; }
    u16 bus_master_port() const { return m_bus_master_port; }

    void wait_while_busy() const;
    void wait_for_irq();
    void poll() const; // An alternative to IRQs

    void prepare_for(ATACommand command, u32 lba, u16 sectors) const;

    bool read_blocks(void* buffer, size_t count, size_t block) override;
    bool write_blocks(const void* buffer, size_t count, size_t block) override;

    void read_sectors(u32 lba, u8 sectors, u8* buffer);
    void write_sectors(u32 lba, u8 sectors, const u8* buffer);

    void read_sectors_with_dma(u32 lba, u8 sectors, u8* buffer);
    void write_sectors_with_dma(u32 lba, u8 sectors, const u8* buffer);
private:
    PATADevice(ATAChannel channel, ATADrive drive, pci::Address address);

    void handle_interrupt(arch::InterruptRegisters*) override;

    ATAChannel m_channel;
    ATADrive m_drive;

    u16 m_control_port;
    u16 m_data_port;
    u16 m_bus_master_port;

    u16 m_cylinders;
    u16 m_heads;
    u16 m_sectors_per_track;

    BooleanBlocker m_irq_blocker;

    bool m_has_48bit_pio;
    bool m_has_dma;

    PhysicalRegionDescriptor* m_prdt = nullptr;
    u8* m_dma_buffer = nullptr;
};

}