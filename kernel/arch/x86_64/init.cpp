#include <kernel/arch/arch.h>
#include <kernel/arch/x86_64/gdt.h>

#include <kernel/common.h>
#include <cpuid.h>

namespace kernel::arch {


void set_interrupt_handler(u32 interrupt, uintptr_t handler, u8 flags) {}

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

void init() {
    enable_sse();

    init_gdt();
}

}