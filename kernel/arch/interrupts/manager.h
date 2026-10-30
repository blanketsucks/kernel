#pragma once

#include <kernel/arch/interrupts/controller.h>

#include <std/vector.h>
#include <std/memory.h>

namespace kernel {

struct InterruptSourceOverride {
    u8  bus;
    u8  irq;
    u32 gsi;
    union {
        struct {
            u8 polarity     : 2;
            u8 trigger_mode : 2;
        };

        u16 flags;
    };
};

class InterruptManager {
public:
    static InterruptManager* instance();
    static void initialize();

    static RefPtr<InterruptController> controller_for(u16 irq);

    Vector<InterruptSourceOverride> const& interrupt_source_overrides() const { return m_iso; }

private:
    RefPtr<InterruptController> find_responsible_controller(u16 irq);

    void init();

    void enumerate_ioapics();

    Vector<RefPtr<InterruptController>> m_controllers;

    Vector<InterruptSourceOverride> m_iso;
};

}