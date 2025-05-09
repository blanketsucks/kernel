#include <limine.h>

#include <kernel/serial.h>
#include <kernel/boot/boot_info.h>

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

static volatile limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

static volatile limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

extern "C" void main(BootInfo const&);

extern "C" void _early_main() {
    asm volatile("cli");

    BootInfo boot_info;

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

    auto* rsdp = rsdp_request.response;
    boot_info.rsdp = rsdp->address;

    Vector<MemoryMapEntry> entries;
    entries.reserve(mmap_request.response->entry_count);

    for (size_t i = 0; i < mmap_request.response->entry_count; i++) {
        limine_memmap_entry* entry = mmap_request.response->entries[i];
        entries.append(MemoryMapEntry {
            .base = entry->base,
            .length = entry->length,
            .type = static_cast<MemoryType>(entry->type + 1)
        });
    }

    boot_info.mmap.count = entries.size();
    boot_info.mmap.entries = entries.data();

    if (framebuffer_request.response->framebuffer_count > 0) {
        auto* fb = framebuffer_request.response->framebuffers[0];

        boot_info.framebuffer.width = fb->width;
        boot_info.framebuffer.height = fb->height;
        boot_info.framebuffer.pitch = fb->pitch;
        boot_info.framebuffer.bpp = fb->bpp;
        boot_info.framebuffer.red_mask_size = fb->red_mask_size;
        boot_info.framebuffer.red_mask_shift = fb->red_mask_shift;
        boot_info.framebuffer.green_mask_size = fb->green_mask_size;
        boot_info.framebuffer.green_mask_shift = fb->green_mask_shift;
        boot_info.framebuffer.blue_mask_size = fb->blue_mask_size;
        boot_info.framebuffer.blue_mask_shift = fb->blue_mask_shift;
        boot_info.framebuffer.address = reinterpret_cast<PhysicalAddress>(fb->address) - boot_info.hhdm;
        boot_info.framebuffer.edid_size = fb->edid_size;
        boot_info.framebuffer.edid = reinterpret_cast<PhysicalAddress>(fb->edid) - boot_info.hhdm;
    }

    main(boot_info);
    __builtin_unreachable();
}