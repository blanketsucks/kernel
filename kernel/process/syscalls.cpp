#include <kernel/process/syscalls.h>
#include <kernel/process/scheduler.h>
#include <kernel/process/threads.h>
#include <kernel/process/process.h>
#include <kernel/arch/interrupts.h>
#include <kernel/arch/registers.h>

namespace kernel {

static int handle_syscall(Process* process, arch::Registers* regs) {
    switch (regs->eax) {
        case SYS_EXIT: {
            process->sys$exit(regs->ebx);
        }
        case SYS_OPEN: {
            return process->sys$open(reinterpret_cast<const char*>(regs->ebx), regs->ecx, regs->edx, regs->esi);
        }
        case SYS_CLOSE: {
            return process->sys$close(regs->ebx);
        }
        case SYS_READ: {
            return process->sys$read(regs->ebx, reinterpret_cast<void*>(regs->ecx), regs->edx);
        }
        case SYS_WRITE: {
            return process->sys$write(regs->ebx, reinterpret_cast<const void*>(regs->ecx), regs->edx);
        }
        default:
            serial::printf("Unknown syscall: %d\n", regs->eax);
    }

    return 0;
}

extern "C" void _syscall_interrupt_handler(arch::InterruptRegisters*);

extern "C" void _syscall_handler(arch::Registers* regs) {
    auto* thread = Scheduler::current_thread();
    auto* process = thread->process();

    regs->eax = handle_syscall(process, regs);
}

void setup_syscall_handler() {
    arch::set_interrupt_handler(0x80, reinterpret_cast<uintptr_t>(_syscall_interrupt_handler), 0xEE);
}

}