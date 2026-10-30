#pragma once

#include <kernel/arch/interrupts/controller.h>

#include <std/memory.h>

namespace kernel {

class PIC : public InterruptController {
public:
    static constexpr u8 MASTER_COMMAND = 0x20;
    static constexpr u8 MASTER_DATA = 0x21;

    static constexpr u8 SLAVE_COMMAND = 0xA0;
    static constexpr u8 SLAVE_DATA = 0xA1;

    static constexpr u8 EOF = 0x20;

    static RefPtr<PIC> create() {
        return RefPtr(new PIC());
    }

    static void disable();

    void enable(u16 irq) override;
    void disable(u16 irq) override;

    u16 gsi_base() const override { return 0; }
    size_t vector_count() const override { return 16; }

    void eoi(u16 irq) override;
    
    StringView name() const override { return "PIC"; }; 

private:
    PIC();

    void initialize();
};

}