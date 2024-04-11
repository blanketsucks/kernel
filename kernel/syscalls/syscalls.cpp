#include <kernel/syscalls/syscalls.h>
#include <kernel/serial.h>

namespace kernel {

extern "C" void _syscall_handler(RegisterState* regs) {
    serial::printf("Syscall: %d\n", regs->eax);
    serial::printf("EBX: %d\n", regs->ebx);

    regs->eax = 42;
    return;
}

void init_syscalls() {
    // cpu::set_idt_entry(0x80, reinterpret_cast<u32>(_syscall_interrupt_handler), 0xEE);
}

}