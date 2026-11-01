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

void wrmsr(u32 msr, u64 value) {
    u32 low = value & 0xFFFFFFFF;
    u32 high = value >> 32;

    asm volatile("wrmsr" :: "a"(low), "d"(high), "c"(msr));
}

u64 rdmsr(u32 msr) {
    u32 low, high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));

    return (static_cast<u64>(high) << 32) | low;
}

}