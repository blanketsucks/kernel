#include <multiboot.h>

#include <kernel/boot/boot_info.h>
#include <std/vector.h>

using namespace kernel;

constexpr VirtualAddress KERNEL_VIRTUAL_BASE = 0xC0000000;
constexpr VirtualAddress KERNEL_PHYSICAL_BASE = 0x100000;

constexpr VirtualAddress KERNEL_HEAP_BASE = 0xE0000000;

extern "C" u64 _kernel_start;
extern "C" u64 _kernel_end;

extern "C" void main(BootInfo const&);

extern "C" void _early_main(multiboot_info_t* header) {
    BootInfo boot_info;

    boot_info.kernel_virtual_base = KERNEL_VIRTUAL_BASE + 0x100000;
    boot_info.kernel_physical_base = KERNEL_PHYSICAL_BASE;
    boot_info.kernel_heap_base = KERNEL_HEAP_BASE;

    boot_info.cmdline = reinterpret_cast<char*>(header->cmdline + KERNEL_VIRTUAL_BASE);
    
    u64 kernel_start = reinterpret_cast<u64>(&_kernel_start);
    u64 kernel_end = reinterpret_cast<u64>(&_kernel_end);

    boot_info.kernel_size = std::align_up(kernel_end - kernel_start, PAGE_SIZE);

    Vector<MemoryMapEntry> entries;
    entries.reserve(header->mmap_length / sizeof(multiboot_memory_map_t));

    for (u32 i = 0; i < header->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* entry = reinterpret_cast<multiboot_memory_map_t*>(header->mmap_addr + KERNEL_VIRTUAL_BASE + i);
        entries.append(MemoryMapEntry {
            .base = entry->addr,
            .length = entry->len,
            .type = static_cast<MemoryType>(entry->type)
        });
    }

    boot_info.mmap.count = entries.size();
    boot_info.mmap.entries = entries.data();

    main(boot_info);
    __builtin_unreachable();
}