#include <kernel/arch/pic.h>
#include <kernel/arch/interrupts.h>
#include <kernel/arch/io.h>
#include <kernel/vga.h>

namespace kernel {

IRQHandler::IRQHandler(u8 irq) : m_irq(irq) {
    pic::enable(irq);
    pic::set_irq_handler(irq, this);
}

void IRQHandler::enable_irq_handler() {
    pic::set_irq_handler(m_irq, this);
}

void IRQHandler::disable_irq_handler() {
    pic::set_irq_handler(m_irq, nullptr); // A bit jank but whatever
}

void IRQHandler::eoi() {
    pic::eoi(m_irq);
    m_did_eoi = true;
}

namespace pic {

static IRQHandler* s_irq_handlers[16] = {};

extern "C" void* _irq_stub_table[];

extern "C" void _irq_handler(arch::InterruptRegisters* regs) {
    u8 irq = regs->intno - 32;
    IRQHandler* handler = s_irq_handlers[irq];

    if (!handler) {
        return eoi(irq);
    }

    handler->handle_interrupt(regs);
    if (handler->did_eoi()) {
        return;
    }

    eoi(irq);
}

void eoi(u8 irq) {
    if (irq >= 8) {
        // Send EOI to both master and slave PIC in this case
        io::write(SLAVE_COMMAND, EOF);
    }

    io::write(MASTER_COMMAND, EOF);
}

void set_irq_handler(u8 irq, IRQHandler* handler) {
    s_irq_handlers[irq] = handler;
}

void set_irq_handler(u8 irq, uintptr_t handler) {
    enable(irq);
    arch::set_interrupt_handler(32 + irq, handler, 0x8E);
}

void disable() {
    io::write<u8>(MASTER_DATA, 0xFF);
    io::write<u8>(SLAVE_DATA, 0xFF);
}

void remap() {
    // Start initialization sequence
    io::write<u8>(MASTER_COMMAND, 0x11);
    io::wait();

    io::write<u8>(SLAVE_COMMAND, 0x11);
    io::wait();

    // Set offsets
    io::write<u8>(MASTER_DATA, 0x20);
    io::wait();

    io::write<u8>(SLAVE_DATA, 0x28);
    io::wait();

    // Set cascade identity
    io::write<u8>(MASTER_DATA, 0x04);
    io::wait();

    io::write<u8>(SLAVE_DATA, 0x02);
    io::wait();

    // Set 8086 mode
    io::write<u8>(MASTER_DATA, 0x01);
    io::wait();
    
    io::write<u8>(SLAVE_DATA, 0x01);
    io::wait();

    // Disable all IRQs
    io::write<u8>(MASTER_DATA, 0xFF);
    io::write<u8>(SLAVE_DATA, 0xFF);

    pic::enable(2); // Enable IRQ 2 which is the slave PIC
}

void init() {
    remap();

    for (u8 i = 0; i < 16; i++) {
        arch::set_interrupt_handler(32 + i, reinterpret_cast<uintptr_t>(_irq_stub_table[i]), 0x8E);
    }
}

void disable(u8 irq) {
    asm volatile("cli");

    u16 port = 0;
    if (irq < 8) {
        port = MASTER_DATA;
    } else {
        port = SLAVE_DATA;
        irq -= 8;
    }

    u8 value = io::read<u8>(port) | (1 << irq);
    io::write(port, value);

    asm volatile("sti");
}

void enable(u8 irq) {
    asm volatile("cli");

    u16 port = 0;
    if (irq < 8) {
        port = MASTER_DATA;
    } else {
        port = SLAVE_DATA;
        irq -= 8;
    }

    u8 value = io::read<u8>(port);
    value &= ~(1 << irq);

    io::write(port, value);
    asm volatile("sti");
}

}

}