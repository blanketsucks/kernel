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

static GDTEntry s_gdt_entries[11];

static GDTEntry s_entries[256];
static u16 s_entry_count = 1;

u16 create_gdt_entry(u32 limit, GDTEntrySize size, GDTEntryFlags flags, u8 privilege, GDTEntryType type) {
    GDTEntry& entry = s_entries[s_entry_count];
    memset(&entry, 0, sizeof(GDTEntry));

    bool is_data_segment = std::has_flag(flags, GDTEntryFlags::DataSegment);

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

    entry.access.readable = std::has_flag(flags, GDTEntryFlags::Readable);
    entry.access.executable = std::has_flag(flags, GDTEntryFlags::Executable);
    entry.access.privilege = privilege;
    entry.access.type = to_underlying(type);
    entry.access.present = true;

    return s_entry_count++;
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

u16 write_tss(TSS& tss) {
    u16 index = s_entry_count;

    u64 base = reinterpret_cast<u64>(&tss);
    u32 limit = sizeof(TSS) - 1;

    GDTEntry entry;

    entry.set_base(base & 0xFFFFFFFF);
    entry.set_limit(limit);

    entry.flags.size = 1;

    entry.access.executable = true;
    entry.access.present = true;
    entry.access.accessed = true;

    s_entries[s_entry_count++] = entry;

    entry.high = 0;
    entry.low = base >> 32;

    s_entries[s_entry_count++] = entry;

    return index;
}

void init_gdt() {
    memset(s_entries, 0, sizeof(s_entries));

    // TODO: Limine forces us to have entries for 16 and 32 bit segments and currently every descriptor index is hardcoded,
    //       so we need to find a way to make this more flexible.
    create_gdt_entry(0xFFFF, GDT_ENTRY_SIZE_16, GDTEntryFlags::Readable | GDTEntryFlags::Executable);
    create_gdt_entry(0xFFFF, GDT_ENTRY_SIZE_16, GDTEntryFlags::DataSegment | GDTEntryFlags::Readable);

    create_gdt_entry(0xFFFFFFFF, GDT_ENTRY_SIZE_32, GDTEntryFlags::Readable | GDTEntryFlags::Executable);
    create_gdt_entry(0xFFFFFFFF, GDT_ENTRY_SIZE_32, GDTEntryFlags::DataSegment | GDTEntryFlags::Readable);

    // Kernel 64-bit code/data
    create_gdt_entry(0, GDT_ENTRY_SIZE_64, GDTEntryFlags::Readable | GDTEntryFlags::Executable);
    create_gdt_entry(0, GDT_ENTRY_SIZE_64, GDTEntryFlags::DataSegment | GDTEntryFlags::Readable);

    // User 64-bit code/data
    create_gdt_entry(0, GDT_ENTRY_SIZE_64, GDTEntryFlags::DataSegment | GDTEntryFlags::Readable, 3);
    create_gdt_entry(0, GDT_ENTRY_SIZE_64, GDTEntryFlags::Readable | GDTEntryFlags::Executable, 3);

    auto& tss = Processor::instance().tss();
    u16 index = write_tss(tss);

    GDTDescriptor gdtr {
        .limit = static_cast<u16>(sizeof(GDTEntry) * s_entry_count - 1),
        .base = reinterpret_cast<u64>(&s_entries)
    };

    _flush_gdt(&gdtr);
    set_tss_selector(index * sizeof(GDTEntry));
}

}