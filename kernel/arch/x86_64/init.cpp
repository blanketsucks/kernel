#include <kernel/common.h>
#include <kernel/arch/arch.h>
#include <kernel/arch/x86_64/gdt.h>
#include <kernel/arch/x86_64/idt.h>
#include <kernel/arch/x86_64/msr.h>

#include <kernel/process/syscalls.h>
#include <kernel/process/scheduler.h>
#include <kernel/process/process.h>
#include <kernel/arch/cpu.h>
#include <std/format.h>

#include <cpuid.h>

namespace kernel::arch {

struct KernelCPUData {
    void* user_stack;
    void* kernel_stack;
};

static volatile KernelCPUData s_cpu_data;
ALIGNED(16) static u8 s_kernel_stack[4096];

// extern "C" void _syscall_handler(Registers* regs) {
//     auto* process = Scheduler::current_process();
//     regs->rax = process->handle_syscall(regs);
// }

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
    wmsr(MSR_SFMASK, 0b1001010111111111010101u); // Disables most bits but preserves reserved and virtualization bits
}

void init() {
    enable_sse();
    enable_syscall();
    
    init_gdt();
    init_idt();

    s_cpu_data.kernel_stack = reinterpret_cast<void*>(s_kernel_stack + sizeof(s_kernel_stack));
    wmsr(MSR_GS_BASE, reinterpret_cast<u64>(&s_cpu_data));
}

}