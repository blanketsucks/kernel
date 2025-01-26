#include <kernel/arch/x86_64/gdt.h>
#include <std/string.h>

namespace kernel::arch {

extern "C" void _flush_gdt(GDTPointer*);

static GDTEntry s_gdt_entries[6];
constexpr u32 LIMIT = 0xFFFFF;

void set_gdt_entry(
    u32 index, u32 base, u32 limit, bool is_data_segment,
    bool rw, bool executable, u8 privilege,
    u8 type, bool present, bool accessed 
) {
    GDTEntry& entry = s_gdt_entries[index];
    memset(&entry, 0, sizeof(GDTEntry));

    entry.base_low = base & 0xFFFF;
    entry.base_middle = (base >> 16) & 0xFF;
    entry.base_high = (base >> 24) & 0xFF;

    entry.limit_low = limit & 0xFFFF;
    entry.flags.limit_high = (limit >> 16) & 0x0F;

    entry.flags.granularity = 1; // Scale limit in 4KB units
    if (!is_data_segment) {
        entry.flags.long_mode = 1;
    } else {
        entry.flags.size = 1;
    }

    entry.access.readable = rw;
    entry.access.executable = executable;
    entry.access.privilege = privilege;
    entry.access.type = type;
    entry.access.present = present;
    entry.access.accessed = accessed;
}

void init_gdt() {
    GDTPointer gp {
        .limit = sizeof(s_gdt_entries) - 1,
        .base = reinterpret_cast<u64>(&s_gdt_entries)
    };

    // Null segment
    memset(&s_gdt_entries[0], 0, sizeof(GDTEntry));

    // Kernel code segment
    set_gdt_entry(1, 0, LIMIT, false, true, true, 0);

    // Kernel data segment
    set_gdt_entry(2, 0, LIMIT, true, true, false, 0);

    // User code segment
    set_gdt_entry(3, 0, LIMIT, false, true, true, 3);

    // User data segment
    set_gdt_entry(4, 0, LIMIT, true, true, false, 3);

    _flush_gdt(&gp);
}

}