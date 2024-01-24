#include <kernel/cpu/cpu.h>
#include <kernel/io.h>

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

}