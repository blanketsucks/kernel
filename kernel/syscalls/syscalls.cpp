#include <kernel/syscalls/syscalls.h>
#include <kernel/vga.h>

namespace kernel {

extern "C" void _syscall_handler(RegisterState* regs) {
    vga::printf("Syscall: %d\n", regs->eax);
    vga::printf("EBX: %d\n", regs->ebx);

    regs->eax = 42;
    return;
}

void init_syscalls() {
    cpu::set_idt_entry(0x80, reinterpret_cast<u32>(__handle_syscall), 0xEE);
}

}