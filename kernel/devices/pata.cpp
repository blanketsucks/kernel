#include <kernel/devices/pata.h>
#include <kernel/cpu/pic.h>
#include <kernel/cpu/idt.h>
#include <kernel/io.h>
#include <kernel/serial.h>
#include <kernel/process/threads.h>
#include <kernel/process/scheduler.h>
#include <kernel/memory/mm.h>

namespace kernel::devices {

u16 operator+(ATARegister lhs, u16 rhs) { return to_underlying(lhs) + rhs; }
u16 operator+(u16 lhs, ATARegister rhs) { return lhs + to_underlying(rhs); }

PATADevice* PATADevice::create(ATAChannel channel, ATADrive drive) {
    pci::Address address = {0};
    pci::enumerate([&address](pci::Device device) {
        if (device.is_ide_controller()) {
            address = device.address;
        }
    });

    if (!address.value) {
        return nullptr;
    }

    return new PATADevice(channel, drive, address);
}

PATADevice::PATADevice(
    ATAChannel channel, ATADrive drive, pci::Address address
) : DiskDevice(3, 0, SECTOR_SIZE),
    IRQHandler(drive == ATADrive::Master ? PRIMARY_IRQ : SECONDARY_IRQ), 
    m_channel(channel), m_drive(drive) {

    m_bus_master_port = address.bar4() & ~1;
    if (channel == ATAChannel::Primary) {
        m_data_port = PRIMARY_DATA_PORT;
        m_control_port = PRIMARY_CONTROL_PORT;
    } else {
        m_data_port = SECONDARY_DATA_PORT;
        m_control_port = SECONDARY_CONTROL_PORT;
    }

    io::write<u8>(m_data_port + ATARegister::Drive, 0xA0 | (to_underlying(drive) << 4));

    io::write<u8>(m_data_port + ATARegister::SectorCount, 0x00);
    io::write<u8>(m_data_port + ATARegister::LBA0, 0x00);
    io::write<u8>(m_data_port + ATARegister::LBA1, 0x00);
    io::write<u8>(m_data_port + ATARegister::LBA2, 0x00);

    io::write<u8>(m_data_port + ATARegister::Command, to_underlying(ATACommand::Identify));

    while (io::read<u8>(m_data_port + ATARegister::Status) & to_underlying(ATAStatus::Busy));

    u8 status = io::read<u8>(m_control_port);
    if (status == 0x00) {
        return;
    }

    u16* buffer = new u16[256];
    for (u16 i = 0; i < 256; i++) {
        buffer[i] = io::read<u16>(m_data_port);
    }

    m_cylinders = buffer[1];
    m_heads = buffer[3];
    m_sectors_per_track = buffer[6];
    m_has_dma = buffer[49] & (1 << 8);

    m_has_48bit_pio = buffer[83] & (1 << 10);
    
    serial::printf("PATA Device Information (%d:%d):\n", to_underlying(m_channel), to_underlying(m_drive));
    serial::printf("  Cylinders: %u\n", m_cylinders);
    serial::printf("  Heads: %u\n", m_heads);
    serial::printf("  Sectors per track: %u\n", m_sectors_per_track);
    serial::printf("  DMA Supported: %s\n", m_has_dma ? "Yes" : "No");
    serial::printf("  48 bit PIO: %s\n", m_has_48bit_pio ? "Yes" : "No");
}

void PATADevice::handle_interrupt(cpu::Registers*) {
    u16 port = m_bus_master_port;
    u8 status = io::read<u8>(port + ATARegister::BusMasterStatus);

    if (!(status & 0x04)) {
        return;
    }

    io::write<u8>(port + ATARegister::BusMasterStatus, status | 0x04);
    m_irq_blocker.set_value(true);
}

size_t PATADevice::max_io_block_count() const {
    return m_has_48bit_pio ? UINT16_MAX : UINT8_MAX;
}

void PATADevice::wait_while_busy() const {
	while (io::read<u8>(m_data_port + ATARegister::Status) & to_underlying(ATAStatus::Busy));
}

void PATADevice::wait_for_irq() {
    this->enable_irq_handler();

	Thread* thread = Scheduler::current_thread();
    thread->block(&m_irq_blocker);

    m_irq_blocker.set_value(false);
    this->disable_irq_handler();
}

void PATADevice::poll() const {
    u8 status = io::read<u8>(m_data_port + ATARegister::Status);
    while (status & to_underlying(ATAStatus::Busy) || !(status & to_underlying(ATAStatus::DataRequest))) {
        status = io::read<u8>(m_data_port + ATARegister::Status);
    }
}

void PATADevice::prepare_for(ATACommand command, u32 lba, u16 sectors) const {
    u16 port = m_data_port;

    this->wait_while_busy();
    if (!m_has_48bit_pio) {
        // Select the drive
        io::write<u8>(port + ATARegister::Drive, 0xE0 | (to_underlying(m_drive) << 4) | ((lba >> 24) & 0x0F));

        // Write the sector count and the LBA (sector) we want to read from
        io::write<u8>(port + ATARegister::SectorCount, sectors);
        io::write<u8>(port + ATARegister::LBA0, lba & 0xFF);
        io::write<u8>(port + ATARegister::LBA1, (lba >> 8) & 0xFF);
        io::write<u8>(port + ATARegister::LBA2, (lba >> 16) & 0xFF);
    } else {
        // Select the drive
        io::write<u8>(port + ATARegister::Drive, 0x40 | (to_underlying(m_drive) << 4));

        // We first write the high byte of the sector count, then the 3 high bytes of the LBA
        io::write<u8>(port + ATARegister::SectorCount, (sectors >> 8) & 0xFF);
        io::write<u8>(port + ATARegister::LBA0, 0x00);
        io::write<u8>(port + ATARegister::LBA1, 0x00);
        io::write<u8>(port + ATARegister::LBA2, (lba >> 24) & 0xFF);

        // Then we write the low byte of the sector count, and the low bytes of the LBA
        io::write<u8>(port + ATARegister::SectorCount, sectors & 0xFF);
        io::write<u8>(port + ATARegister::LBA0, lba & 0xFF);
        io::write<u8>(port + ATARegister::LBA1, (lba >> 8) & 0xFF);
        io::write<u8>(port + ATARegister::LBA2, (lba >> 16) & 0xFF);

        if (command == ATACommand::Read) {
            command = ATACommand::ReadExt;
        } else if (command == ATACommand::Write) {
            command = ATACommand::WriteExt;
        }
    }

    this->wait_while_busy();
    io::write(port + ATARegister::Command, to_underlying(command));
}

void PATADevice::read_sectors(u32 lba, u8 count, u8* buffer) {
    this->prepare_for(ATACommand::Read, lba, count);

    for (u8 i = 0; i < count; i++) {
        this->poll();

        for (u16 j = 0; j < 256; j++) {
            u16 data = io::read<u16>(m_data_port + ATARegister::Data);

            buffer[j * 2] = u8(data);
            buffer[j * 2 + 1] = u8(data >> 8);
        }

        buffer += SECTOR_SIZE;
    }
}

void PATADevice::write_sectors(u32 lba, u8 count, const u8* buffer) {
    this->prepare_for(ATACommand::Write, lba, count);

    for (u8 i = 0; i < count; i++) {
        this->poll();

        for (u16 j = 0; j < 256; j++) {
            u16 data = buffer[j * 2] | (buffer[j * 2 + 1] << 8);
            io::write<u16>(m_data_port + ATARegister::Data, data);
        }

        buffer += SECTOR_SIZE;
    }
}

void PATADevice::read_sectors_with_dma(u32, u8, u8*) {
    // TODO: Implement DMA
    return;
}

void PATADevice::write_sectors_with_dma(u32, u8, const u8*) {
    // TODO: Implement DMA
    return;
}

bool PATADevice::read_blocks(void* buffer, size_t count, size_t block) {
    if (count > this->max_io_block_count()) {
        return false;
    }

    this->read_sectors(block, count, reinterpret_cast<u8*>(buffer));
    return true;
}

bool PATADevice::write_blocks(const void* buffer, size_t count, size_t block) {
    if (count > this->max_io_block_count()) {
        return false;
    }

    this->write_sectors(block, count, reinterpret_cast<const u8*>(buffer));
    return true;
}

}