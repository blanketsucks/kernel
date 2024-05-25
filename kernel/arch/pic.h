#pragma once

#include <kernel/common.h>
#include <kernel/arch/registers.h>

namespace kernel {

class IRQHandler {
public:
    IRQHandler(u8 irq);

    virtual void handle_interrupt(arch::InterruptRegisters*) = 0;

    void enable_irq_handler();
    void disable_irq_handler();

    u8 irq() const { return m_irq; }
    bool did_eoi() const { return m_did_eoi; }
    
    void eoi();

private:
    u8 m_irq;
    bool m_did_eoi = false;
};

}

namespace kernel::pic {

constexpr u8 MASTER_COMMAND = 0x20;
constexpr u8 MASTER_DATA = 0x21;

constexpr u8 SLAVE_COMMAND = 0xA0;
constexpr u8 SLAVE_DATA = 0xA1;

constexpr u8 EOF = 0x20;

void disable();
void remap();

void init();

void set_irq_handler(u8 irq, IRQHandler* handler);
void set_irq_handler(u8 irq, uintptr_t handler);

void eoi(u8 irq);

void disable(u8 irq);
void enable(u8 irq);


}