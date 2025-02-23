#pragma once

#include <kernel/common.h>

#include <kernel/arch/x86/page_directory.h>
#include <kernel/arch/x86/arch.h>

namespace kernel {
    class Stack;
    class Thread;
}

namespace kernel::arch {

struct InterruptRegisters {
    u32 gs, fs, es, ds;
    u32 edi, esi, ebp, esp0, ebx, edx, ecx, eax;
    u32 intno, errno;
    u32 eip, cs, eflags, esp, ss;

    FlatPtr ip() const { return eip; }
    FlatPtr flags() const { return eflags; }
} PACKED;

struct InterruptFrame {
    u32 eip, cs, eflags, esp, ss;
} PACKED;

struct ThreadRegisters {
    u32 edi, esi, ebp, esp0, ebx, edx, ecx, eax;
    u32 eip, cs, eflags, esp, ss;
    u32 cr3;

    void set_flags(FlatPtr flags) { eflags = flags; }

    void set_user_sp(FlatPtr sp) { esp = sp; }
    void set_kernel_sp(FlatPtr sp) { esp0 = sp; }

    void set_ip(FlatPtr ip) { eip = ip; }
    void set_bp(FlatPtr bp) { ebp = bp; }

    FlatPtr flags() const { return eflags; }
    FlatPtr ip() const { return eip; }
    FlatPtr sp() const { return esp; }
    FlatPtr sp0() const { return esp0; }

    void set_syscall_return(FlatPtr value) { eax = value; }

    void set_initial_stack_state(Thread*);
} PACKED;

struct Registers {
    u32 gs, fs, es, ds;
    u32 edi, esi, ebp, esp0, ebx, edx, ecx, eax;
    u32 eip, cs, eflags, esp, ss;

    void capture_syscall_arguments(FlatPtr& syscall, FlatPtr& arg1, FlatPtr& arg2, FlatPtr& arg3, FlatPtr& arg4) {
        syscall = eax;
        arg1 = ebx;
        arg2 = ecx;
        arg3 = edx;
        arg4 = esi;
    }
} PACKED;

void dump_registers(const Registers&);
void dump_registers(const InterruptRegisters&);

}