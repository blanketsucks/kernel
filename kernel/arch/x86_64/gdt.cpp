#include <kernel/arch/x86_64/gdt.h>
#include <kernel/arch/x86_64/tss.h>
#include <kernel/arch/processor.h>

#include <kernel/process/scheduler.h>

#include <std/cstring.h>
#include <std/format.h>

namespace kernel::arch {

extern "C" void _flush_gdt(GDTDescriptor*);

static void set_tss_selector(u16 selector) {
    asm volatile("ltr %0" :: "r"(selector));
}


// Limine requires us to have at least 7 GDT entries and we use the rest for user segments
static GDTEntry s_gdt_entries[11];

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

void write_tss(u16 index, TSS& tss) {
    u64 base = reinterpret_cast<u64>(&tss);
    u32 limit = sizeof(TSS) - 1;

    GDTEntry entry;

    entry.set_base(base & 0xFFFFFFFF);
    entry.set_limit(limit);

    entry.flags.size = 1;

    entry.access.executable = true;
    entry.access.present = true;
    entry.access.accessed = true;

    s_gdt_entries[index] = entry;

    entry.high = 0;
    entry.low = base >> 32;

    s_gdt_entries[index + 1] = entry;
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

    // User 64 bit segments
    set_gdt_entry(7, 0, true, GDT_ENTRY_SIZE_64, true, false, 3);
    set_gdt_entry(8, 0, false, GDT_ENTRY_SIZE_64, true, true, 3);

    auto& tss = Processor::instance().tss();
    write_tss(9, tss);

    _flush_gdt(&gdtr);
    set_tss_selector(0x48);
}

}