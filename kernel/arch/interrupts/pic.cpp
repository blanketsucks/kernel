#include <kernel/arch/interrupts/pic.h>
#include <kernel/arch/interrupts.h>
#include <kernel/arch/io.h>

namespace kernel {

PIC::PIC() {
    initialize();
}

void PIC::disable() {
    io::write<u8>(MASTER_DATA, 0xFF);
    io::write<u8>(SLAVE_DATA, 0xFF);
}

void PIC::initialize() {
    // Start initialization sequence
    io::write<u8>(MASTER_COMMAND, 0x11);
    io::wait();

    io::write<u8>(SLAVE_COMMAND, 0x11);
    io::wait();

    // Set offsets
    io::write<u8>(MASTER_DATA, IRQ_VECTOR_BASE);
    io::wait();

    io::write<u8>(SLAVE_DATA, IRQ_VECTOR_BASE + 8);
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

    enable(2); // Enable IRQ 2 which is the slave PIC
}

void PIC::disable(u16 irq) {
    arch::InterruptDisabler disabler;

    u16 port = 0;
    if (irq < 8) {
        port = MASTER_DATA;
    } else {
        port = SLAVE_DATA;
        irq -= 8;
    }

    u8 value = io::read<u8>(port) | (1 << irq);
    io::write(port, value);
}

void PIC::enable(u16 irq) {
    arch::InterruptDisabler disabler;

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
}

void PIC::eoi(u16 irq) {
    if (irq >= 8) {
        // Send EOI to both master and slave PIC in this case
        io::write(SLAVE_COMMAND, EOF);
    }

    io::write(MASTER_COMMAND, EOF);
}

}