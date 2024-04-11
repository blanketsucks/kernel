#pragma once

#include <kernel/common.h>
#include <kernel/cpu/idt.h>

namespace kernel {

struct RegisterState {
    u32 gs, fs, es, ds;
    u32 edi, esi, ebp, esp, ebx, edx, ecx, eax;
    u32 eip, cs, eflags, esp0, ss;
};

extern "C" void _syscall_interrupt_handler(cpu::InterruptFrame*);
extern "C" void _syscall_handler(RegisterState* regs);

void init_syscalls();

}