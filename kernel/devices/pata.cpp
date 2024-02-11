#include <kernel/devices/pata.h>
#include <kernel/cpu/pic.h>
#include <kernel/cpu/idt.h>

#include <kernel/io.h>
#include <kernel/serial.h>

namespace kernel::devices {

u16 operator+(ATARegister lhs, u16 rhs) { return to_underlying(lhs) + rhs; }
u16 operator+(u16 lhs, ATARegister rhs) { return lhs + to_underlying(rhs); }

PATADevice* s_device_instance = nullptr;

INTERRUPT void irq14_handler(cpu::InterruptFrame* frame) {
    (void)frame;

    u16 port = s_device_instance->bus_master_port();
    u8 status = io::read<u8>(port + ATARegister::BusMasterStatus);

    if (!(status & 0x04)) return;

    io::write<u8>(port + ATARegister::BusMasterStatus, status | 0x04);
    s_device_instance->set_irq_state(true);

    pic::eoi(14);
}

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
) : DiskDevice(3, 0, SECTOR_SIZE), m_channel(channel), m_drive(drive) {
    s_device_instance = this;
    m_bus_master_port = pci::read<u32>(address, 0x20) & (~1);

    u8 irq = drive == ATADrive::Master ? 14 : 15;

    pic::enable(irq);
    cpu::set_idt_entry(irq + 32, reinterpret_cast<u32>(irq14_handler), 0x8E);

    u16 data_port = this->data_port();
    u16 control_port = this->control_port();

    io::write<u8>(data_port + ATARegister::Drive, 0xA0 | (to_underlying(drive) << 4));

    io::write<u8>(data_port + ATARegister::SectorCount, 0x00);
    io::write<u8>(data_port + ATARegister::LBA0, 0x00);
    io::write<u8>(data_port + ATARegister::LBA1, 0x00);
    io::write<u8>(data_port + ATARegister::LBA2, 0x00);

    io::write<u8>(data_port + ATARegister::Command, to_underlying(ATACommand::Identify));

    while (io::read<u8>(data_port + ATARegister::Status) & to_underlying(ATAStatus::Busy));

    u8 status = io::read<u8>(control_port);
    if (status == 0x00) return;

    u16* buffer = new u16[256];
    for (u16 i = 0; i < 256; i++) {
        buffer[i] = io::read<u16>(data_port);
    }

    m_cylinders = buffer[1];
    m_heads = buffer[3];
    m_sectors_per_track = buffer[6];

    m_has_48bit_pio = buffer[83] & (1 << 10);

    serial::printf("PATA Device Information (%d:%d):\n", to_underlying(m_channel), to_underlying(m_drive));
    serial::printf("  Cylinders: %u\n", m_cylinders);
    serial::printf("  Heads: %u\n", m_heads);
    serial::printf("  Sectors per track: %u\n", m_sectors_per_track);
    serial::printf("  48 bit PIO: %s\n", m_has_48bit_pio ? "Yes" : "No");
}

u16 PATADevice::control_port() const {
    switch (m_channel) {
        case ATAChannel::Primary:
            return PRIMARY_CONTROL_PORT;
        case ATAChannel::Secondary:
            return SECONDARY_CONTROL_PORT;
    }
}

u16 PATADevice::data_port() const {
    switch (m_channel) {
        case ATAChannel::Primary:
            return PRIMARY_DATA_PORT;
        case ATAChannel::Secondary:
            return SECONDARY_DATA_PORT;
    }
}

void PATADevice::wait_while_busy() const {
	while (io::read<u8>(this->data_port() + ATARegister::Status) & to_underlying(ATAStatus::Busy));
}

void PATADevice::wait_for_irq() {
	while (!m_did_irq);
}

void PATADevice::poll() const {
    u8 status = io::read<u8>(this->data_port() + ATARegister::Status);
    while (status & to_underlying(ATAStatus::Busy) || !(status & to_underlying(ATAStatus::DataRequest))) {
        status = io::read<u8>(this->data_port() + ATARegister::Status);
    }
}

void PATADevice::prepare_for(ATACommand command, u32 lba, u16 sectors) const {
    u16 port = this->data_port();

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

void PATADevice::read_sectors(u32 lba, u8 count, u8* buffer) const {
    this->prepare_for(ATACommand::Read, lba, count);

    for (u8 i = 0; i < count; i++) {
        this->poll();

        for (u16 j = 0; j < 256; j++) {
            u16 data = io::read<u16>(this->data_port() + ATARegister::Data);

            buffer[j * 2] = u8(data);
            buffer[j * 2 + 1] = u8(data >> 8);
        }

        buffer += SECTOR_SIZE;
    }
}

void PATADevice::write_sectors(u32 lba, u8 count, const u8* buffer) const {
    this->prepare_for(ATACommand::Write, lba, count);

    for (u8 i = 0; i < count; i++) {
        this->poll();

        for (u16 j = 0; j < 256; j++) {
            u16 data = buffer[j * 2] | (buffer[j * 2 + 1] << 8);
            io::write<u16>(this->data_port() + ATARegister::Data, data);
        }

        buffer += SECTOR_SIZE;
    }
}

bool PATADevice::read_blocks(void* buffer, size_t count, size_t block) {
    if (count > UINT8_MAX) {
        // In ATA 28 bit PIO, we can only read up to 255 sectors at a time.
        return false;
    }

    this->read_sectors(block, count, reinterpret_cast<u8*>(buffer));
    return true;
}

bool PATADevice::write_blocks(const void* buffer, size_t count, size_t block) {
    if (count > UINT8_MAX) {
        // In ATA 28 bit PIO, we can only write up to 255 sectors at a time.
        return false;
    }

    this->write_sectors(block, count, reinterpret_cast<const u8*>(buffer));
    return true;
}

}