#include <kernel/arch/x86/gdt.h>
#include <kernel/arch/x86/tss.h>
#include <kernel/process/scheduler.h>

#include <std/cstring.h>

namespace kernel::arch {

static GDTEntry s_gdt_entries[6];
constexpr u32 LIMIT = 0xFFFFF;

extern "C" void _flush_gdt(GDTDescriptor*);
extern "C" void _flush_tss();

void write_tss(u16 index, TSS& tss) {
    u32 base = reinterpret_cast<u32>(&tss);
    u32 limit = sizeof(TSS);

    auto& entry = s_gdt_entries[index];
    memset(&entry, 0, sizeof(GDTEntry));

    entry.base_low = base & 0xFFFF;
    entry.base_middle = (base >> 16) & 0xFF;
    entry.base_high = (base >> 24) & 0xFF;

    entry.limit_low = limit & 0xFFFF;
    entry.flags.limit_high = (limit >> 16) & 0x0F;

    entry.flags.granularity = 0;
    entry.flags.size = 0;

    entry.access.readable = false;
    entry.access.executable = true;
    entry.access.privilege = 0;
    entry.access.type = 0;
    entry.access.present = true;
    entry.access.accessed = true;
}

void set_gdt_entry(
    u32 index, u32 base, u32 limit, 
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
    entry.flags.size = 1;

    entry.access.readable = rw;
    entry.access.executable = executable;
    entry.access.privilege = privilege;
    entry.access.type = type;
    entry.access.present = present;
    entry.access.accessed = accessed;
}

void init_gdt() {
    GDTDescriptor gdtr {
        .limit = sizeof(s_gdt_entries) - 1,
        .base = reinterpret_cast<u32>(&s_gdt_entries)
    };

    set_gdt_entry(0, 0, 0, false, false, 0, 0, false); // Null segment
    set_gdt_entry(1, 0, LIMIT, true, true, 0);  // Kernel code segment
    set_gdt_entry(2, 0, LIMIT, true, false, 0); // Kernel data segment
    set_gdt_entry(3, 0, LIMIT, true, true, 3);  // User code segment
    set_gdt_entry(4, 0, LIMIT, true, false, 3);  // User data segment

    auto& tss = Scheduler::tss();
    write_tss(5, tss);

    _flush_gdt(&gdtr);
    _flush_tss();
}

}