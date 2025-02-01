#include <kernel/arch/boot_info.h>
#include <kernel/multiboot.h>

#include <std/vector.h>

using namespace kernel;

extern "C" u64 _kernel_start;
extern "C" u64 _kernel_end;

extern "C" void main(arch::BootInfo const&);

extern "C" void _early_main(multiboot_info_t* header) {
    arch::BootInfo boot_info;

    boot_info.kernel_virtual_base = 0xC0100000;
    boot_info.kernel_physical_base = 0x100000;
    
    u64 kernel_start = reinterpret_cast<u64>(&_kernel_start);
    u64 kernel_end = reinterpret_cast<u64>(&_kernel_end);

    boot_info.kernel_size = std::align_up(kernel_end - kernel_start, PAGE_SIZE);

    Vector<arch::MemoryMapEntry> entries;
    entries.reserve(header->mmap_length / sizeof(multiboot_memory_map_t));

    for (u32 i = 0; i < header->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* entry = reinterpret_cast<multiboot_memory_map_t*>(header->mmap_addr + KERNEL_VIRTUAL_BASE + i);
        entries.append(arch::MemoryMapEntry {
            .base = entry->addr,
            .length = entry->len,
            .type = static_cast<arch::MemoryType>(entry->type)
        });
    }

    boot_info.mmap.count = entries.size();
    boot_info.mmap.entries = entries.data();

    main(boot_info);
    __builtin_unreachable();
}