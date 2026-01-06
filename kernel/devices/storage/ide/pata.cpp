#include <kernel/devices/storage/ide/pata.h>
#include <kernel/process/scheduler.h>
#include <kernel/process/threads.h>
#include <kernel/memory/manager.h>

#include <std/format.h>

namespace kernel {

RefPtr<PATADevice> PATADevice::create(ata::Channel channel, ata::Drive drive, pci::Address address) {
    auto device = RefPtr<PATADevice>(new PATADevice(channel, drive, address));
    if (!device->max_addressable_block()) { // If max_addressable_block is 0, we returned early in the constructor which means the device is not present
        return nullptr;
    }

    Device::after_device_creation(device);
    return device;
}

PATADevice::PATADevice(
    ata::Channel channel, ata::Drive drive, pci::Address address
) : StorageDevice(SECTOR_SIZE),
    IRQHandler(drive == ata::Drive::Master ? ata::PRIMARY_IRQ : ata::SECONDARY_IRQ), 
    m_channel(channel), m_drive(drive) {
    m_bus_master = address.bar(4) & ~1;
    if (channel == ata::Channel::Primary) {
        m_data = ata::PRIMARY_DATA_PORT;
        m_control = ata::PRIMARY_CONTROL_PORT;
    } else {
        m_data = ata::SECONDARY_DATA_PORT;
        m_control = ata::SECONDARY_CONTROL_PORT;
    }

    m_data.write<u8>(ata::DriveReg, 0xA0 | (to_underlying(drive) << 4));

    m_data.write<u8>(ata::SectorCount, 0x00);
    m_data.write<u8>(ata::LBA0, 0x00);
    m_data.write<u8>(ata::LBA1, 0x00);
    m_data.write<u8>(ata::LBA2, 0x00);

    m_data.write<u8>(ata::CommandReg, ata::Identify);

    u8 status = m_data.read<u8>(ata::StatusReg);
    while (status & ata::Busy) {
        status = m_data.read<u8>(ata::StatusReg);
    }

    if (status & ata::Error || status == 0) {
        return;
    }

    u8 cl = m_data.read<u8>(ata::LBA1);
    u8 ch = m_data.read<u8>(ata::LBA2);

    if (cl == 0x14 && ch == 0xEB) {
        m_type = PATAPI;

        m_data.write<u8>(ata::CommandReg, ata::IdentifyPacket);
        while (m_data.read<u8>(ata::StatusReg) & ata::Busy);

        // TODO: Implement ATAPI
    } else {
        m_type = PATA;
    }

    u16* buffer = new u16[256];
    for (u16 i = 0; i < 256; i++) {
        buffer[i] = m_data.read<u16>();
    }

    auto* identify = reinterpret_cast<ata::IdentifyData*>(buffer);

    m_has_dma = identify->capabilities[0] & ata::DMASupported;
    m_has_48bit_pio = identify->command_sets_supported[1] & ata::LBA48Bit;

    if (m_has_48bit_pio) {
        m_max_addressable_block = identify->lba_48_max_addressable_block;
    } else {
        m_max_addressable_block = identify->lba_28_max_addressable_block;
    }

    if (m_has_dma) {
        auto* mm = MemoryManager::instance();

        address.set_bus_master(true);
        address.set_interrupt_line(true);

        u8 status = m_bus_master.read<u8>(ata::BMStatus);
        m_bus_master.write<u8>(ata::BMStatus, status | 0x04);

        m_prdt = reinterpret_cast<PhysicalRegionDescriptor*>(MUST(mm->allocate_dma_region(sizeof(PhysicalRegionDescriptor))));
        m_dma_buffer = reinterpret_cast<u8*>(MUST(mm->allocate_dma_region(DMA_BUFFER_SIZE)));

        m_prdt->base = mm->get_physical_address(m_dma_buffer);

        this->enable_irq();
    }

    size_t capacity = m_max_addressable_block * SECTOR_SIZE;
    
    dbgln("PATA Device Information ({}:{}):", to_underlying(m_channel), to_underlying(m_drive));
    dbgln(" - DMA Supported: {}", m_has_dma);
    dbgln(" - Has 48 bit PIO: {}", m_has_48bit_pio);
    dbgln(" - Max Addressable Sector: {}", m_max_addressable_block);
    dbgln(" - Capacity: {} MB\n", capacity / MB);
}

void PATADevice::handle_irq() {
    u8 status = m_bus_master.read<u8>(ata::BMStatus);
    if (!(status & 0x04)) {
        return;
    }
    
    m_bus_master.write<u8>(ata::BMStatus, status | 0x04);
    m_irq_blocker.set_value(true);
}

size_t PATADevice::max_io_block_count() const {
    if (m_has_dma) {
        return DMA_BUFFER_SIZE / SECTOR_SIZE;
    }

    return m_has_48bit_pio ? UINT16_MAX : UINT8_MAX;
}

void PATADevice::wait_while_busy() const {
    while (m_data.read<u8>(ata::StatusReg) & ata::Busy);
}

void PATADevice::wait_for_irq() {
    Thread* thread = Thread::current();
    thread->block(&m_irq_blocker);
}

void PATADevice::poll() const {
    u8 status = m_data.read<u8>(ata::StatusReg);
    while (status & ata::Busy || !(status & ata::DataRequest)) {
        status = m_data.read<u8>(ata::StatusReg);
    }
}

void PATADevice::prepare_for(ata::Command command, size_t lba, u16 sectors) {
    this->wait_while_busy();
    if (!m_has_48bit_pio) {
        // Select the drive
        m_data.write<u8>(ata::DriveReg, 0xE0 | (to_underlying(m_drive) << 4) | ((lba >> 24) & 0x0F));

        // Write the sector count and the LBA (sector) we want to read from
        m_data.write<u8>(ata::SectorCount, sectors & 0xFF);
        m_data.write<u8>(ata::LBA0, lba & 0xFF);
        m_data.write<u8>(ata::LBA1, (lba >> 8) & 0xFF);
        m_data.write<u8>(ata::LBA2, (lba >> 16) & 0xFF);
    } else {
        // Select the drive
        m_data.write<u8>(ata::DriveReg, 0x40 | (to_underlying(m_drive) << 4));

        // We first write the high byte of the sector count, then the 3 high bytes of the LBA
        m_data.write<u8>(ata::SectorCount, (sectors >> 8) & 0xFF);
        
        if constexpr (sizeof(size_t) > 4) {
            m_data.write<u8>(ata::LBA0, (lba >> 40) & 0xFF);
            m_data.write<u8>(ata::LBA1, (lba >> 32) & 0xFF);
        } else {
            m_data.write<u8>(ata::LBA0, 0x00);
            m_data.write<u8>(ata::LBA1, 0x00);
        }

        m_data.write<u8>(ata::LBA2, (lba >> 24) & 0xFF);

        // Then we write the low byte of the sector count, and the low bytes of the LBA
        m_data.write<u8>(ata::SectorCount, sectors & 0xFF);

        m_data.write<u8>(ata::LBA0, lba & 0xFF);
        m_data.write<u8>(ata::LBA1, (lba >> 8) & 0xFF);
        m_data.write<u8>(ata::LBA2, (lba >> 16) & 0xFF);

        switch (command) {
            case ata::Read:
                command = ata::ReadExt; break;
            case ata::Write:
                command = ata::WriteExt; break;
            case ata::ReadDMA:
                command = ata::ReadDMAExt; break;
            case ata::WriteDMA:
                command = ata::WriteDMAExt; break;
            default: break;
        }
    }

    this->wait_while_busy();
    m_data.write<u8>(ata::CommandReg, command);
}

void PATADevice::read_sectors(size_t lba, u8 count, u8* buffer) {
    this->prepare_for(ata::Read, lba, count);

    for (u8 i = 0; i < count; i++) {
        this->poll();

        for (u16 j = 0; j < 256; j++) {
            u16 data = m_data.read<u16>(ata::Data);

            buffer[j * 2] = u8(data);
            buffer[j * 2 + 1] = u8(data >> 8);
        }

        buffer += SECTOR_SIZE;
    }
}

void PATADevice::write_sectors(size_t lba, u8 count, const u8* buffer) {
    this->prepare_for(ata::Write, lba, count);

    for (u8 i = 0; i < count; i++) {
        this->poll();

        for (u16 j = 0; j < 256; j++) {
            u16 data = buffer[j * 2] | (buffer[j * 2 + 1] << 8);
            m_data.write<u16>(ata::Data, data);
        }

        buffer += SECTOR_SIZE;
    }
}

void PATADevice::read_sectors_with_dma(size_t lba, u8 count, u8* buffer) {
    m_irq_blocker.set_value(false);

    m_prdt->size = count * SECTOR_SIZE;
    m_prdt->flags = 0x8000;

    m_data.write<u8>(ata::DriveReg, 0x40 | (to_underlying(m_drive) << 4));
    m_bus_master.write<u8>(0);

    m_bus_master.write<u32>(ata::BMPRDT, MM->get_physical_address(m_prdt));
    m_bus_master.write<u8>(ata::BMRead);

    u8 status = m_bus_master.read<u8>(ata::BMStatus);
    m_bus_master.write<u8>(ata::BMStatus, status | 0x06);

    this->prepare_for(ata::ReadDMA, lba, count);

    while (!(m_control.read<u8>() & to_underlying(ata::DataRequest)));

    m_bus_master.write<u8>(0x09);
    this->wait_for_irq();

    memcpy(buffer, m_dma_buffer, count * SECTOR_SIZE); 

    status = m_bus_master.read<u8>(ata::BMStatus);
    m_bus_master.write<u8>(ata::BMStatus, status | 0x06);
}

void PATADevice::write_sectors_with_dma(size_t, u8, const u8*) {
    // TODO: Implement DMA
    return;
}

ErrorOr<bool> PATADevice::read_blocks(void* buffer, size_t count, size_t block) {
    if (count > this->max_io_block_count()) {
        return Error(EINVAL);
    }

    this->read_sectors_with_dma(block, count, reinterpret_cast<u8*>(buffer));
    return true;
}

ErrorOr<bool> PATADevice::write_blocks(const void* buffer, size_t count, size_t block) {
    if (count > this->max_io_block_count()) {
        return Error(EINVAL);
    }

    this->write_sectors(block, count, reinterpret_cast<const u8*>(buffer));
    return true;
}

}