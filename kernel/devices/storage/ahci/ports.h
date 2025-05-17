#pragma once

#include <kernel/common.h>
#include <kernel/devices/storage/ahci/ahci.h>
#include <kernel/devices/storage/ata.h>
#include <kernel/process/blocker.h>

#include <std/memory.h>
#include <std/result.h>

namespace kernel {

class AHCIController;
class SATADevice;

class AHCIPort {
public:
    static constexpr size_t COMMAND_LIST_SIZE = 32;

    static constexpr size_t MAX_PRDT_COUNT = 8;
    static constexpr size_t PRDT_BUFFER_SIZE = 4096;

    static RefPtr<AHCIPort> create(ahci::HBAPort* port, u32 index, AHCIController* AHCIController) {
        return RefPtr<AHCIPort>(new AHCIPort(port, index, AHCIController));
    }

    bool initialize();

    u32 index() const { return m_index; }

    ahci::PortSignature signature() const { return (ahci::PortSignature)m_port->signature; }
    RefPtr<SATADevice> device() const { return m_device; }

    bool is_ata_port() const { return signature() == ahci::PortSignature::ATA; }
    bool is_atapi_port() const { return signature() == ahci::PortSignature::ATAPI; }

    bool is_active() const;

    void wait_while_busy();
    void issue_command(int slot);

    int prepare_for(ata::Command, u64 lba, u16 sectors);

    ErrorOr<void> read_sectors(u64 lba, u16 count, u8* buffer);
    ErrorOr<void> write_sectors(u64 lba, u16 count, const u8* buffer);
    
private:
    friend AHCIController;
    
    AHCIPort(ahci::HBAPort* port, u32 index, AHCIController* AHCIController) : m_port(port), m_controller(AHCIController), m_index(index) {}
    
    void clear_interrupt_enable() { m_port->interrupt_enable = 0; }
    void set_interrupt_enable() { m_port->interrupt_enable = 0xffffffff; }
    
    void clear_interrupt_status() { m_port->interrupt_status = 0xffffffff; }
    
    void rebase();
    void allocate_resources();
    bool identify();

    void start();
    void stop();

    int find_free_command_slot();

    void handle_interrupt();

    ahci::HBAPort* m_port;
    AHCIController* m_controller;

    u32 m_index;

    ahci::CommandHeader* m_command_headers;
    ahci::CommandTable* m_command_tables;

    void* m_fis_receive;

    u8* m_prdt_buffer;
    u8* m_identify_buffer;

    RefPtr<SATADevice> m_device;

    BooleanBlocker m_irq_blocker;
};

}