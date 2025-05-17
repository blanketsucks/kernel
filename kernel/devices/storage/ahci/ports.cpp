#include <kernel/devices/storage/ahci/ports.h>
#include <kernel/devices/storage/ahci/ahci.h>
#include <kernel/devices/storage/ahci/sata.h>
#include <kernel/devices/storage/ata.h>
#include <kernel/memory/manager.h>
#include <kernel/process/scheduler.h>


#include <std/format.h>

namespace kernel {

using namespace ahci;

static constexpr size_t SIZE_OF_COMMAND_TABLES = (sizeof(CommandTable) + sizeof(PhysicalRegionDescriptor) * AHCIPort::MAX_PRDT_COUNT) * AHCIPort::COMMAND_LIST_SIZE;

void AHCIPort::allocate_resources() {
    m_command_headers = reinterpret_cast<CommandHeader*>(MM->allocate_dma_region(sizeof(CommandHeader) * COMMAND_LIST_SIZE));
    m_command_tables = reinterpret_cast<CommandTable*>(MM->allocate_dma_region(SIZE_OF_COMMAND_TABLES));
    m_fis_receive = MM->allocate_dma_region(PAGE_SIZE);

    m_prdt_buffer = reinterpret_cast<u8*>(MM->allocate_dma_region(PRDT_BUFFER_SIZE * MAX_PRDT_COUNT));
    m_identify_buffer = reinterpret_cast<u8*>(MM->allocate_kernel_region(PAGE_SIZE));

    for (size_t i = 0; i < COMMAND_LIST_SIZE; i++) {
        auto& header = m_command_headers[i];
        auto* table = &m_command_tables[i];
        
        memset(&header, 0, sizeof(CommandHeader));
        memset(table, 0, sizeof(CommandTable) + sizeof(PhysicalRegionDescriptor) * MAX_PRDT_COUNT);
        
        PhysicalAddress address = MM->get_physical_address(table);
        
        header.command_table_base = address;
        if constexpr (sizeof(address) > 4) {
            header.command_table_base_upper = (address >> 32) & 0xFFFFFFFF;
        }
        
        header.prdt_length = MAX_PRDT_COUNT;
    }
 
    memset(m_fis_receive, 0, PAGE_SIZE);
    memset(m_identify_buffer, 0, PAGE_SIZE);
}

void AHCIPort::rebase() {
    m_port->fis_base = MM->get_physical_address(m_fis_receive);
    m_port->fis_base_upper = 0;
    m_port->command_list = MM->get_physical_address(m_command_headers);
    m_port->command_list_upper = 0;
}

bool AHCIPort::initialize() {
    if (!this->is_active()) {
        return false;
    }

    this->stop();

    this->allocate_resources();
    this->rebase();

    m_port->sata_error = 1;
    m_port->cmd = (m_port->cmd & 0x0fffffff) | (1 << 28);

    this->clear_interrupt_status();
    this->set_interrupt_enable();

    this->start();

    if (this->identify()) {
        auto* identify = reinterpret_cast<ata::IdentifyData*>(m_identify_buffer);

        u64 max_addressable_block = 0;
        if (identify->command_sets_supported[1] & ata::LBA48Bit) {
            max_addressable_block = identify->lba_48_max_addressable_block;
        } else {
            max_addressable_block = identify->lba_28_max_addressable_block;
        }

        dbgln(" - AHCI Port {}: Device Signature: {:#x}, Max Addressable Block: {}", m_index, to_underlying(signature()), max_addressable_block);
        m_device = SATADevice::create(this, max_addressable_block);
    }

    return true;
}

bool AHCIPort::is_active() const {
    u8 det = (m_port->sata_status >> 0) & 0xF;
    u8 ipm = (m_port->sata_status >> 8) & 0xF;

    // 3: Device is detected and communication is established
    // 1: Device is in active state
    return det == 3 && ipm == 1;
}

void AHCIPort::start() {
    while (m_port->cmd & PxCMD::CR) {}

    m_port->cmd |= PxCMD::FRE;
    m_port->cmd |= PxCMD::ST;
}

void AHCIPort::stop() {
    m_port->cmd &= ~PxCMD::ST;
    m_port->cmd &= ~PxCMD::FRE;

    while (m_port->cmd & (PxCMD::FR | PxCMD::CR)) {}
}

int AHCIPort::find_free_command_slot() {
    u32 ci = m_port->command_issue;
    for (size_t i = 0; i < COMMAND_LIST_SIZE; i++) {
        if (!(ci & (1 << i))) {
            return i;
        }
    }

    return -1;
}

bool AHCIPort::identify() {
    int slot = this->find_free_command_slot();
    if (slot < 0) {
        return false;
    }

    auto& header = m_command_headers[slot];
    auto* table = &m_command_tables[slot];

    memset(table, 0, sizeof(CommandTable) + sizeof(PhysicalRegionDescriptor) * MAX_PRDT_COUNT);

    header.prdb_count = 512;
    header.prdt_length = 1;
    header.fis_length = sizeof(FISRegisterH2D) / sizeof(u32);
    header.write = 0;
    header.prefetchable = 1;

    table->prdt[0].data_base_upper = 0;
    table->prdt[0].data_base = MM->get_physical_address(m_identify_buffer);
    table->prdt[0].byte_count = 512 - 1;

    auto* fis = reinterpret_cast<volatile FISRegisterH2D*>(table->command_fis);

    fis->type = FISRegisterH2D::H2D;
    fis->command = ata::Command::Identify;
    fis->device = 1;
    fis->c = 1;

    while (m_port->task_file_data & (ata::Busy | ata::DataRequest)) {}

    this->clear_interrupt_enable();
    this->clear_interrupt_status();

    m_port->command_issue = 1 << slot;
    while (true) {
        if (!(m_port->command_issue & (1 << slot))) {
            break;
        } else if (m_port->interrupt_status & PxIS::TFES) {
            return false;
        } else if (m_port->sata_error != 0) {
            return false;
        }
    }

    if (m_port->interrupt_status & PxIS::TFES) {
        return false;
    }

    this->clear_interrupt_status();
    this->set_interrupt_enable();

    return true;
}

void AHCIPort::wait_while_busy() {
    while (m_port->task_file_data & (ata::Busy | ata::DataRequest)) {}
}

void AHCIPort::issue_command(int slot) {
    m_irq_blocker.set_value(false);

    m_port->command_issue = 1 << slot;
    m_irq_blocker.wait();
}

int AHCIPort::prepare_for(ata::Command command, u64 lba, u16 sectors) {
    int slot = this->find_free_command_slot();
    if (slot < 0) {
        return -1;
    }

    auto& header = m_command_headers[slot];
    auto* table = &m_command_tables[slot];

    memset(table, 0, sizeof(CommandTable) + sizeof(PhysicalRegionDescriptor) * MAX_PRDT_COUNT);

    header.prdt_length = MAX_PRDT_COUNT;
    header.fis_length = sizeof(FISRegisterH2D) / sizeof(u32);

    if (command == ata::ReadDMAExt) {
        header.write = 0;
    } else {
        header.write = 1;
    }

    for (size_t i = 0; i < MAX_PRDT_COUNT; i++) {
        table->prdt[i].data_base_upper = 0;
        table->prdt[i].data_base = MM->get_physical_address(m_prdt_buffer + i * PRDT_BUFFER_SIZE);
        table->prdt[i].byte_count = PRDT_BUFFER_SIZE - 1;

        table->prdt[i].interrupt_on_completion = 1;
    }

    auto* fis = reinterpret_cast<volatile FISRegisterH2D*>(table->command_fis);

    fis->type = FISRegisterH2D::H2D;
    fis->command = command;
    fis->device = 1 << 6;
    fis->c = 1;

    fis->lba0 = lba & 0xFF;
    fis->lba1 = (lba >> 8) & 0xFF;
    fis->lba2 = (lba >> 16) & 0xFF;

    fis->lba3 = (lba >> 24) & 0xFF;
    fis->lba4 = (lba >> 32) & 0xFF;
    fis->lba5 = (lba >> 40) & 0xFF;

    fis->count_low = sectors & 0xFF;
    fis->count_high = (sectors >> 8) & 0xFF;

    return slot;
}

ErrorOr<void> AHCIPort::read_sectors(u64 lba, u16 sectors, u8* buffer) {
    int slot = this->prepare_for(ata::ReadDMAExt, lba, sectors);
    if (slot < 0) {
        return Error(EBUSY);
    }

    this->wait_while_busy();
    this->issue_command(slot);

    // FIXME: Check for errors
    // FIXME: Don't hardcode the 512 sector size
    memcpy(buffer, m_prdt_buffer, sectors * 512);
    return {};
}

ErrorOr<void> AHCIPort::write_sectors(u64 lba, u16 sectors, const u8* buffer) {
    int slot = this->prepare_for(ata::WriteDMAExt, lba, sectors);
    if (slot < 0) {
        return Error(EBUSY);
    }

    memcpy(m_prdt_buffer, buffer, sectors * SECTOR_SIZE);

    this->wait_while_busy();
    this->issue_command(slot);

    // FIXME: Check for errors
    return {};
}


void AHCIPort::handle_interrupt() {
    u32 status = m_port->interrupt_status;
    if (status == 0) {
        return;
    }

    m_irq_blocker.set_value(true);
    this->clear_interrupt_status();
}

}