#include "kernel/common.h"
#include <kernel/arch/apic.h>
#include <kernel/memory/manager.h>
#include <kernel/serial.h>

namespace kernel::apic {

static VirtualAddress g_apic_base;

PhysicalAddress get_apic_base() {
    u32 low, high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(IA32_APIC_BASE));

    return low & 0xfffff000;
}

void set_apic_base(PhysicalAddress base) {
    u32 low = (base & 0xfffff0000) | IA32_APIC_BASE_ENABLE;
    u32 high = 0;

    asm volatile("wrmsr" :: "a"(low), "d"(high), "c"(IA32_APIC_BASE));
}

void write_reg(APICRegisters reg, u32 value) {
    write_reg(to_underlying(reg), value);
}

void write_reg(u32 reg, u32 value) {
    *reinterpret_cast<volatile u32*>(g_apic_base + reg) = value;
}

u32 read_reg(APICRegisters reg) {
    return read_reg(to_underlying(reg));
}

u32 read_reg(u32 reg) {
    return *reinterpret_cast<volatile u32*>(g_apic_base + reg);
}

void init() {
    PhysicalAddress base = get_apic_base();
    set_apic_base(base);

    // Map the APIC registers
    g_apic_base = reinterpret_cast<VirtualAddress>(MM->map_physical_region(reinterpret_cast<void*>(base), PAGE_SIZE));

    u32 value = read_reg(APICRegisters::SpuriousInterruptVector);
    write_reg(APICRegisters::SpuriousInterruptVector, value | SPURIOUS_INTERRUPT_VECTOR | 0x100);
}


}