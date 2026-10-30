#pragma once

#include <kernel/arch/interrupts/controller.h>

#include <std/memory.h>

namespace kernel {

class IOAPIC : public InterruptController {
public:
    static RefPtr<IOAPIC> create(PhysicalAddress address, u16 gsi_base) {
        return RefPtr(new IOAPIC(address, gsi_base));
    }

    void enable(u16) override {}
    void disable(u16) override {}

    u16 gsi_base() const override { return m_gsi_base; }
    size_t vector_count() const override { return 16; }

    void eoi(u16) override {}
    
    StringView name() const override { return "IOAPIC"; }; 

private:
    IOAPIC(PhysicalAddress, u16) {}

    u16 m_gsi_base;
};

}