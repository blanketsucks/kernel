#include <kernel/arch/x86_64/gdt.h>

#include <std/cstring.h>
#include <std/format.h>

namespace kernel::arch {

extern "C" void _flush_gdt(GDTDescriptor*);

// Limine requires us to have at least 7 GDT entries and we use the rest for user segments
static GDTEntry s_gdt_entries[9];

void set_gdt_entry(
    u32 index,
    u32 limit,
    bool is_data_segment,
    GDTEntrySize size,
    bool rw,
    bool executable,
    u8 privilege,
    u8 type,
    bool present,
    bool accessed 
) {
    GDTEntry& entry = s_gdt_entries[index];
    memset(&entry, 0, sizeof(GDTEntry));

    entry.limit_low = limit & 0xFFFF;
    entry.flags.limit_high = (limit >> 16) & 0xF;

    entry.flags.granularity = 1; // Scale limit in 4KB units
    switch (size) {
        case GDT_ENTRY_SIZE_32:
            entry.flags.size = 1;
            break;
        case GDT_ENTRY_SIZE_64:
            if (!is_data_segment) {
                entry.flags.long_mode = 1;
            } else {
                entry.flags.size = 1;
            }
            break;
        case GDT_ENTRY_SIZE_16: // We do nothing here as entry.flags.size is already set to 0
            break;
    }

    entry.access.readable = rw;
    entry.access.executable = executable;
    entry.access.privilege = privilege;
    entry.access.type = type;
    entry.access.present = present;
    entry.access.accessed = accessed;
}

void dump_entry(size_t index) {
    GDTEntry& entry = s_gdt_entries[index];

    dbgln("GDT Entry {}:", index);
    dbgln("  Access: {:#x}", entry.access.value);
    dbgln("  Flags: {:#x}", entry.flags.value);
}

void init_gdt() {
    GDTDescriptor gdtr {
        .limit = sizeof(s_gdt_entries) - 1,
        .base = reinterpret_cast<u64>(&s_gdt_entries)
    };

    // Null segment
    memset(&s_gdt_entries[0], 0, sizeof(GDTEntry));

    // 16 bit segments
    set_gdt_entry(1, 0xFFFF, false, GDT_ENTRY_SIZE_16, true, true, 0);
    set_gdt_entry(2, 0xFFFF, true, GDT_ENTRY_SIZE_16, true, false, 0);

    // 32 bit segments
    set_gdt_entry(3, 0xFFFFFFFF, false, GDT_ENTRY_SIZE_32, true, true, 0);
    set_gdt_entry(4, 0xFFFFFFFF, true, GDT_ENTRY_SIZE_32, true, false, 0);
    
    // Kernel 64 bit segments
    set_gdt_entry(5, 0, false, GDT_ENTRY_SIZE_64, true, true, 0);
    set_gdt_entry(6, 0, true, GDT_ENTRY_SIZE_64, true, false, 0);

    // User segments
    set_gdt_entry(7, 0, false, GDT_ENTRY_SIZE_64, true, true, 3);
    set_gdt_entry(8, 0, true, GDT_ENTRY_SIZE_64, true, false, 3);

    _flush_gdt(&gdtr);
}

}