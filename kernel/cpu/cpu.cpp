#include <kernel/cpu/cpu.h>
#include <kernel/io.h>
#include <kernel/serial.h>

#include <cpuid.h>

namespace kernel::cpu {

void reset() {
    while (io::read<u8>(0x64) & 0x02) {}
    io::write<u8>(0x64, 0xFE);
}

bool has_feature(EDXFeature feature) {
    u32 eax, ebx, ecx, edx = 0;
    __get_cpuid(1, &eax, &ebx, &ecx, &edx);

    i32 flag = to_underlying(feature);
    return static_cast<i32>(edx & flag) == flag;
}

bool has_feature(ECXFeature feature) {
    u32 eax, ebx, ecx, edx = 0;
    __get_cpuid(1, &eax, &ebx, &ecx, &edx);

    i32 flag = to_underlying(feature);
    return static_cast<i32>(ecx & flag) == flag;
}

bool enable_fpu() {
    CPUID cpuid(1);
    if (!(cpuid.edx() & 0x00000001)) {
        return false;
    }

    asm volatile("fninit");
    return true;
}

bool enable_sse() {
    CPUID cpuid(1);
    if (!(cpuid.edx() & 0x02000000)) {
        return false;
    }

    u32 cr0 = 0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));

    cr0 &= 0xFFFB; // Clear coprocessor emulation CR0.EM 
    cr0 |= 0x2;    // Set coprocessor monitoring CR0.MP

    asm volatile("mov %0, %%cr0" : : "r"(cr0));

    u32 cr4 = 0;
    asm volatile("mov %%cr4, %0" : "=r"(cr4));

    cr4 |= 0x600; // Set CR4.OSFXSR and CR4.OSXMMEXCPT
    asm volatile("mov %0, %%cr4" : : "r"(cr4));

    return true;
}

void save_fpu_state(FPUState &state) {
    asm volatile("fxsave %0" : "=m"(state.buffer));
}

void restore_fpu_state(FPUState &state) {
    asm volatile("fxrstor %0" : : "m"(state.buffer));
}

}