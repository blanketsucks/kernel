#include <kernel/arch/x86_64/msr.h>

namespace kernel::arch {

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

}