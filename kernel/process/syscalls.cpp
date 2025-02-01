#if 0

#include <kernel/process/syscalls.h>
#include <kernel/process/scheduler.h>
#include <kernel/process/threads.h>
#include <kernel/process/process.h>
#include <kernel/arch/interrupts.h>
#include <kernel/arch/registers.h>
#include <kernel/posix/sys/mman.h>

#include <std/format.h>

namespace kernel {

static int handle_syscall(Thread* thread, arch::Registers* regs) {
    auto* process = thread->process();

    switch (regs->eax) {
        case SYS_EXIT: {
            process->sys$exit(regs->ebx);
        }
        case SYS_OPEN: {
            return process->sys$open(reinterpret_cast<const char*>(regs->ebx), regs->ecx, regs->edx);
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
        case SYS_FSTAT: {
            return process->sys$fstat(regs->ebx, reinterpret_cast<stat*>(regs->ecx));
        }
        case SYS_MMAP: {
            auto* args = reinterpret_cast<mmap_args*>(regs->ebx);
            return reinterpret_cast<int>(process->sys$mmap(args->addr, args->size, args->prot, args->flags, args->fd, args->offset));
        }
        case SYS_GETPID: {
            return process->id();
        }
        case SYS_GETPPID: {
            return process->parent_id();
        }
        case SYS_GETTID: {
            return thread->id();
        }
        case SYS_DUP: {
            return process->sys$dup(regs->ebx);
        }
        case SYS_DUP2: {
            return process->sys$dup2(regs->ebx, regs->ecx);
        }
        case SYS_GETCWD: {
            return process->sys$getcwd(reinterpret_cast<char*>(regs->ebx), regs->ecx);
        }
        case SYS_CHDIR: {
            return process->sys$chdir(reinterpret_cast<const char*>(regs->ebx));
        }
        case SYS_IOCTL: {
            return process->sys$ioctl(regs->ebx, regs->ecx, regs->edx);
        }
        case SYS_FORK: {
            return process->sys$fork(*regs);
        }
        case SYS_YIELD: {
            Scheduler::yield();
            return 0;
        }
        default:
            dbgln("Unknown syscall {}", regs->eax);
    }

    return 0;
}

extern "C" void _syscall_handler(arch::Registers* regs) {
    auto* thread = Scheduler::current_thread();
    regs->eax = handle_syscall(thread, regs);
}

}

#endif