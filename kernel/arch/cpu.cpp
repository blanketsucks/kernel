#include <kernel/arch/cpu.h>
#include <cpuid.h>

namespace kernel::arch {

void cpuid(u32 leaf, u32& eax, u32& ebx, u32& ecx, u32& edx) {
    __get_cpuid(leaf, &eax, &ebx, &ecx, &edx);
}

FlatPtr cpu_flags() {
    FlatPtr flags = 0;

#ifdef __x86_64__
    asm volatile(
        "pushfq\n"
        "pop %0\n"
        : "=r"(flags)
    );
#else
    asm volatile(
        "pushf\n"
        "pop %0\n"
        : "=r"(flags)
    );
#endif

    return flags;
}

void wmsr(u32 msr, u64 value) {
    u32 low = value & 0xFFFFFFFF;
    u32 high = value >> 32;

    asm volatile("wrmsr" :: "a"(low), "d"(high), "c"(msr));
}

u64 rmsr(u32 msr) {
    u32 low, high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));

    return (static_cast<u64>(high) << 32) | low;
}

CPUFeatures cpu_features() {
    u32 eax, ebx, ecx, edx;
    __get_cpuid(1, &eax, &ebx, &ecx, &edx);

    return static_cast<CPUFeatures>((static_cast<u64>(edx) << 32) | eax);
}

FlatPtr read_cr0() {
    FlatPtr cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));

    return cr0;
}

FlatPtr read_cr2() {
    FlatPtr cr2;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));

    return cr2;
}

FlatPtr read_cr3() {
    FlatPtr cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));

    return cr3;
}

FlatPtr read_cr4() {
    FlatPtr cr4;
    asm volatile("mov %%cr4, %0" : "=r"(cr4));

    return cr4;
}

void write_cr0(FlatPtr value) {
    asm volatile("mov %0, %%cr0" :: "r"(value));
}

void write_cr2(FlatPtr value) {
    asm volatile("mov %0, %%cr2" :: "r"(value));
}

void write_cr3(FlatPtr value) {
    asm volatile("mov %0, %%cr3" :: "r"(value));
}

void write_cr4(FlatPtr value) {
    asm volatile("mov %0, %%cr4" :: "r"(value));
}

void invlpg(FlatPtr address) {
    asm volatile("invlpg (%0)" :: "r"(address) : "memory");
}

void fxsave(FPUState& state) {
    asm volatile("fxsave %0" :: "m"(state) : "memory");
}

void fxrstor(FPUState& state) {
    asm volatile("fxrstor %0" :: "m"(state) : "memory");
}

}