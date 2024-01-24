#include <kernel/cpu/gdt.h>

namespace kernel::cpu {

static GDTEntry gdt[5];

extern "C" void _flush_gdt(GDTPointer* gp);

void set_gdt_entry(u32 index, u32 base, u32 limit, u8 access, u8 granularity) {
    GDTEntry& entry = gdt[index];

    entry.base_low = base & 0xFFFF;
    entry.base_middle = (base >> 16) & 0xFF;
    entry.base_high = (base >> 24) & 0xFF;

    entry.limit_low = limit & 0xFFFF;
    entry.granularity = (limit >> 16) & 0x0F;

    entry.granularity |= granularity & 0xF0;
    entry.access = access;
}

void init_gdt() {
    
    set_gdt_entry(0, 0, 0, 0, 0); // Null descriptor
    set_gdt_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
    set_gdt_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment
    set_gdt_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User mode code segment
    set_gdt_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data segment

    GDTPointer gp {
        .limit = sizeof(gdt) - 1,
        .base = reinterpret_cast<u32>(&gdt)
    };

    _flush_gdt(&gp);
}

}