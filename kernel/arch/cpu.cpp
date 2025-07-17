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

}