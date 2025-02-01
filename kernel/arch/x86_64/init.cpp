#include <kernel/common.h>
#include <kernel/arch/arch.h>
#include <kernel/arch/x86_64/gdt.h>
#include <kernel/arch/x86_64/idt.h>
#include <kernel/arch/x86_64/msr.h>

#include <std/format.h>

#include <cpuid.h>

namespace kernel::arch {

// TODO: This needs to be per-cpu once we have SMP.
static volatile void* s_user_stack;

extern "C" void _syscall_handler(GPRegisters* regs) {
    dbgln("Syscall: {}", regs->rax);
}

extern "C" void _syscall_interrupt_handler();

void enable_sse() {
    u32 eax, ebx, ecx, edx;
    __get_cpuid(1, &eax, &ebx, &ecx, &edx);
    if (!(edx & (1 << 25))) {
        return;
    }

    asm volatile("mov %cr0, %rax");
    asm volatile("and $0xFFFB, %ax");
    asm volatile("or $2, %ax");
    asm volatile("mov %rax, %cr0");

    asm volatile("mov %cr4, %rax");
    asm volatile("or $0x600, %ax");
    asm volatile("mov %rax, %cr4");
}

void enable_syscall() {
    u32 eax, ebx, ecx, edx;
    __get_cpuid(1, &eax, &ebx, &ecx, &edx);
    if (!(edx & (1 << 11))) {
        return;
    }

    wmsr(MSR_EFER, rmsr(MSR_EFER) | 1);

    wmsr(MSR_STAR, (0x28ul << 32) | (0x33ul << 48));
    wmsr(MSR_LSTAR, reinterpret_cast<u64>(&_syscall_interrupt_handler));

    // wmsr(MSR_GS_BASE, reinterpret_cast<u64>(&s_user_stack));
}

void init() {
    enable_sse();
    enable_syscall();

    init_gdt();
    init_idt();
}

}