#pragma once

#include <kernel/arch/interrupts/controller.h>

#include <std/memory.h>
#include <std/optional.h>

namespace kernel {

union RedirectionEntry {
    struct {
        u8 vector;
        u8 delivery_mode: 3;
        u8 destination_mode: 1;
        u8 delivery_status: 1;
        u8 pin_polarity: 1;
        u8 remote_irr: 1;
        u8 trigger_mode : 1;
        u8 mask: 1;
        u64 : 39;
        u8 destination;
    } PACKED;

    struct {
        u32 low;
        u32 high;
    };
};

class IOAPIC : public InterruptController {
public:
    enum Register : u32 {
        ID = 0,
        Version = 1,
        ArbitrationID = 2,
        RedirectionEntryBase = 0x10
    };

    static RefPtr<IOAPIC> create(PhysicalAddress address, u16 gsi_base) {
        return RefPtr(new IOAPIC(address, gsi_base));
    }

    void enable(u16) override;
    void disable(u16) override;

    u16 gsi_base() const override { return m_gsi_base; }
    size_t vector_count() const override { return m_max_redirection_entry; }

    void eoi(u16) override;
    
    StringView name() const override { return "IOAPIC"; }; 

private:
    IOAPIC(PhysicalAddress, u16 gsi_base);

    u32 read_reg(u32 reg);
    void write_reg(u32 reg, u32 value);

    RedirectionEntry read_redirection_entry(u8 index);
    void write_redirection_entry(u8 index, RedirectionEntry);

    void update_redirection_entry(u8 index, u8 vector, bool pin_polarity, bool trigger_mode, bool mask);

    void set_redirection_entry_mask(u8 index, bool value);
    u8 get_redirection_entry_vector(u8 index);

    u8 configure_interrupt_redirection_entry(u16 irq, bool default_mask = true);

    Optional<u8> find_redirection_entry(u16 irq); 

    VirtualAddress m_base;
    u16 m_gsi_base;

    u8 m_id;
    u8 m_version;
    u16 m_max_redirection_entry;
};

}