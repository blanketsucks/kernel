#include <kernel/cpu/pic.h>
#include <kernel/io.h>
#include <kernel/vga.h>

constexpr u8 PIC_MASTER_COMMAND = 0x20;
constexpr u8 PIC_MASTER_DATA = 0x21;

constexpr u8 PIC_SLAVE_COMMAND = 0xA0;
constexpr u8 PIC_SLAVE_DATA = 0xA1;

constexpr u8 PIC_EOF = 0x20;

namespace kernel::pic {

void send_eoi(u8 irq) {
    if (irq >= 8) {
        // Send EOI to both master and slave PIC in this case
        io::write(PIC_SLAVE_COMMAND, PIC_EOF);
    }

    io::write(PIC_MASTER_COMMAND, PIC_EOF);
}

void disable() {
    io::write<u8>(PIC_MASTER_DATA, 0xFF);
    io::write<u8>(PIC_SLAVE_DATA, 0xFF);
}

void remap() {
    // Start initialization sequence
    io::write<u8>(PIC_MASTER_COMMAND, 0x11);
    io::wait();

    io::write<u8>(PIC_SLAVE_COMMAND, 0x11);
    io::wait();

    // Set offsets
    io::write<u8>(PIC_MASTER_DATA, 0x20);
    io::wait();

    io::write<u8>(PIC_SLAVE_DATA, 0x28);
    io::wait();

    // Set cascade identity
    io::write<u8>(PIC_MASTER_DATA, 0x04);
    io::wait();

    io::write<u8>(PIC_SLAVE_DATA, 0x02);
    io::wait();

    // Set 8086 mode
    io::write<u8>(PIC_MASTER_DATA, 0x01);
    io::wait();
    
    io::write<u8>(PIC_SLAVE_DATA, 0x01);
    io::wait();

    // Disable all IRQs
    io::write<u8>(PIC_MASTER_DATA, 0xFF);
    io::write<u8>(PIC_SLAVE_DATA, 0xFF);

    pic::enable(2); // Enable IRQ 2 which is the slave PIC
}

void disable(u8 irq) {
    u16 port = 0;
    if (irq < 8) {
        port = PIC_MASTER_DATA;
    } else {
        port = PIC_SLAVE_DATA;
        irq -= 8;
    }

    u8 value = io::read<u8>(port) | (1 << irq);
    io::write(port, value);
}

void enable(u8 irq) {
    u16 port = 0;
    if (irq < 8) {
        port = PIC_MASTER_DATA;
    } else {
        port = PIC_SLAVE_DATA;
        irq -= 8;
    }

    u8 value = io::read<u8>(port);
    value &= ~(1 << irq);

    io::write(port, value);
}

}