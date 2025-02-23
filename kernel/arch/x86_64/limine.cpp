#include <limine.h>

#include <kernel/serial.h>
#include <kernel/arch/boot_info.h>

#include <std/vector.h>

using namespace kernel;

extern "C" u64 _kernel_start;
extern "C" u64 _kernel_end;

[[gnu::used]] static volatile LIMINE_BASE_REVISION(1);

static volatile limine_memmap_request mmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

static volatile limine_kernel_address_request kernel_address_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0
};

static volatile limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

static volatile limine_kernel_file_request kernel_file_request = {
    .id = LIMINE_KERNEL_FILE_REQUEST,
    .revision = 0
};

[[gnu::unused]] static volatile limine_stack_size_request stack_size_request = {
    .id = LIMINE_STACK_SIZE_REQUEST,
    .revision = 0,
    .stack_size = 0x100000 / 2
};

extern "C" void main(arch::BootInfo const&);

extern "C" void _early_main() {
    asm volatile("cli");

    arch::BootInfo boot_info;

    auto* kernel_address = kernel_address_request.response;
    
    boot_info.kernel_physical_base = kernel_address->physical_base;
    boot_info.kernel_virtual_base = kernel_address->virtual_base;
    boot_info.kernel_heap_base = kernel_address->virtual_base + GB;

    u64 kernel_start = reinterpret_cast<u64>(&_kernel_start);
    u64 kernel_end = reinterpret_cast<u64>(&_kernel_end);

    boot_info.kernel_size = std::align_up(kernel_end - kernel_start, PAGE_SIZE);
    boot_info.cmdline = kernel_file_request.response->kernel_file->cmdline;

    auto* hhdm = hhdm_request.response;
    boot_info.hhdm = hhdm->offset;

    Vector<arch::MemoryMapEntry> entries;
    entries.reserve(mmap_request.response->entry_count);

    for (size_t i = 0; i < mmap_request.response->entry_count; i++) {
        limine_memmap_entry* entry = mmap_request.response->entries[i];
        entries.append(arch::MemoryMapEntry {
            .base = entry->base,
            .length = entry->length,
            .type = static_cast<arch::MemoryType>(entry->type + 1)
        });
    }

    boot_info.mmap.count = entries.size();
    boot_info.mmap.entries = entries.data();

    main(boot_info);
    __builtin_unreachable();
}