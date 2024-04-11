#include <kernel/cpu/gdt.h>

#include <std/cstring.h>

namespace kernel::cpu {

constexpr u32 LIMIT = 0xFFFFFFFF;

static GDTEntry s_gdt_entries[5];

extern "C" void _flush_gdt(GDTPointer* gp);
extern "C" void _flush_tss();

void set_gdt_entry(
    u32 index, u32 base, u32 limit, 
    bool rw, bool executable, u8 privilege,
    u8 type, bool present, bool accessed 
) {
    GDTEntry& entry = s_gdt_entries[index];
    std::memset(&entry, 0, sizeof(GDTEntry));

    entry.base_low = base & 0xFFFF;
    entry.base_middle = (base >> 16) & 0xFF;
    entry.base_high = (base >> 24) & 0xFF;

    entry.limit_low = limit & 0xFFFF;
    entry.flags.limit_high = (limit >> 16) & 0x0F;

    entry.flags.granularity = 1; // Scale limit in 4KB units
    entry.flags.size = 1;

    entry.access.readable = rw;
    entry.access.executable = executable;
    entry.access.privilege = privilege;
    entry.access.type = type;
    entry.access.present = present;
    entry.access.accessed = accessed;
}

void write_tss(u32 index, TSS& tss) {
    u32 base = reinterpret_cast<u32>(&tss);
    u32 limit = sizeof(TSS);

    GDTEntry& entry = s_gdt_entries[index];

    entry.base_low = base & 0xFFFF;
    entry.base_middle = (base >> 16) & 0xFF;
    entry.base_high = (base >> 24) & 0xFF;
    
    entry.limit_low = limit & 0xFFFF;
    entry.flags.limit_high = (limit >> 16) & 0x0F;

    entry.flags.granularity = 0; // Scale limit in 1 byte units
    entry.flags.size = 0;

    entry.access.present = true;
    entry.access.readable = false;
    entry.access.executable = true;
    entry.access.privilege = 0;
    entry.access.type = 0;
    entry.access.accessed = true;
}

void init_gdt() {
    GDTPointer gp {
        .limit = sizeof(s_gdt_entries) - 1,
        .base = reinterpret_cast<u32>(&s_gdt_entries)
    };

    set_gdt_entry(0, 0, 0, false, false, 0, 0, false); // Null segment
    set_gdt_entry(1, 0, LIMIT, true, true, 0);  // Kernel code segment
    set_gdt_entry(2, 0, LIMIT, true, false, 0); // Kernel data segment
    set_gdt_entry(3, 0, LIMIT, true, true, 3);  // User code segment
    set_gdt_entry(4, 0, LIMIT, true, false, 3);  // User data segment

    _flush_gdt(&gp);
}

}