#include <kernel/io.h>

namespace kernel {

template<> u8 io::read<u8>(u16 port) {
    u8 data = 0;
    asm volatile("inb %1, %0" : "=a"(data) : "Nd"(port));

    return data;
}

template<> u16 io::read<u16>(u16 port) {
    u16 data = 0;
    asm volatile("inw %1, %0" : "=a"(data) : "Nd"(port));

    return data;
}

template<> u32 io::read<u32>(u16 port) {
    u32 data = 0;
    asm volatile("inl %1, %0" : "=a"(data) : "Nd"(port));

    return data;
}

template<> void io::write<u8>(u16 port, u8 data) {
    asm volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

template<> void io::write<u16>(u16 port, u16 data) {
    asm volatile("outw %0, %1" : : "a"(data), "Nd"(port));
}

template<> void io::write<u32>(u16 port, u32 data) {
    asm volatile("outl %0, %1" : : "a"(data), "Nd"(port));
}

void io::wait() { io::write<u8>(0x80, 0); }
void io::wait(u32 us) { while (--us) io::wait(); }

}
