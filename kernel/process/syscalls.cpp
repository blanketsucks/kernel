#include <kernel/process/syscalls.h>
#include <kernel/process/scheduler.h>
#include <kernel/process/threads.h>
#include <kernel/process/process.h>
#include <kernel/arch/interrupts.h>
#include <kernel/arch/registers.h>

namespace kernel {

extern "C" void _syscall_interrupt_handler(arch::InterruptRegisters*);

extern "C" void _syscall_handler(arch::Registers* regs) {
    auto* thread = Scheduler::current_thread();
    auto* process = thread->process();

    u32 result = 0;
    switch (regs->eax) {
        case 0: {
            process->sys$exit(regs->ebx);
            break;
        }
        default:
            serial::printf("Unknown syscall: %d\n", regs->eax);
            break;
    }

    regs->eax = result;
}

void setup_syscall_handler() {
    arch::set_interrupt_handler(0x80, reinterpret_cast<uintptr_t>(_syscall_interrupt_handler), 0xEE);
}

}