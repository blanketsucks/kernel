#pragma once

#include <kernel/common.h>

#include <kernel/arch/x86_64/page_directory.h>
#include <kernel/arch/x86_64/arch.h>

namespace kernel {
    class Thread;
}

namespace kernel::arch {

struct GPRegisters {
    u64 r15, r14, r13, r12, r11, r10, r9, r8;
    u64 rsi, rdi, rbp, rsp0, rdx, rcx, rbx, rax;
} PACKED;

struct InterruptRegisters {
    u64 r15, r14, r13, r12, r11, r10, r9, r8;
    u64 rsi, rdi, rbp, rsp0, rdx, rcx, rbx, rax;
    u64 intno, errno;
    u64 rip, cs, rflags, rsp, ss;

    FlatPtr flags() const { return rflags; }
    FlatPtr ip() const { return rip; }
} PACKED;

struct InterruptFrame {
    u64 rip, cs, rflags, rsp, ss;
} PACKED;

struct ThreadRegisters {
    u64 r15, r14, r13, r12, r11, r10, r9, r8;
    u64 rsi, rdi, rbp, rsp0, rdx, rcx, rbx, rax;

    u64 rip, cs, rflags, rsp, ss;
    u64 cr3;

    void set_flags(FlatPtr flags) { rflags = flags; }

    void set_user_sp(FlatPtr sp) { rsp = sp; }
    void set_kernel_sp(FlatPtr sp) { rsp0 = sp; }

    void set_ip(FlatPtr ip) { rip = ip; }
    void set_bp(FlatPtr bp) { rbp = bp; }

    void set_entry_data(FlatPtr data) {
        rdi = data;
    }

    FlatPtr flags() const { return rflags; }
    FlatPtr ip() const { return rip; }
    FlatPtr sp() const { return rsp; }
    FlatPtr sp0() const { return rsp0; }

    void set_syscall_return(FlatPtr value) { rax = value; }

    void set_initial_stack_state(Thread*);
} PACKED;

struct Registers {
    u64 r15, r14, r13, r12, r11, r10, r9, r8;
    u64 rsi, rdi, rbp, rsp0, rdx, rcx, rbx, rax;
    u64 rip, cs, rflags, rsp, ss;

    void capture_syscall_arguments(FlatPtr& syscall, FlatPtr& arg1, FlatPtr& arg2, FlatPtr& arg3, FlatPtr& arg4) {
        syscall = rax;
        arg1 = rdx;
        arg2 = rdi;
        arg3 = rbx;
        arg4 = rsi;
    }
} PACKED;

}