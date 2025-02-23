#include <kernel/arch/cpu.h>
#include <cpuid.h>

namespace kernel::arch {

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

CPUFeatures cpu_features() {
    u32 eax, ebx, ecx, edx;
    __get_cpuid(1, &eax, &ebx, &ecx, &edx);

    return static_cast<CPUFeatures>((static_cast<u64>(edx) << 32) | eax);
}

}