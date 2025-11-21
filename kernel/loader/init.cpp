#include <multiboot.h>
#include <cpuid.h>

#include <kernel/common.h>
#include <kernel/boot/boot_info.h>
#include <kernel/serial.h>

#include <std/elf.h>
#include <std/cstring.h>
#include <std/utility.h>

using namespace kernel;

extern "C" u64 boot_pml4t[512];
extern "C" u64 boot_pdpt[512];
extern "C" u64 boot_pd[4][512];
extern "C" u64 kernel_pdpt[512];
extern "C" u64 kernel_pd[2][512];

extern "C" u64 _loader_start;
extern "C" u64 _loader_end;

[[gnu::noreturn]] static void abort(const char* str) {
    u16* address = reinterpret_cast<u16*>(0xB8000);
    for (size_t i = 0; i < std::strlen(str); i++) {
        address[i] = 0x0700 | str[i];
    }

    asm volatile("cli");
    while (1) {
        asm volatile("hlt");
    }
}

static MemoryMapEntry mmap_entries[256];
static size_t mmap_count = 0;

static void mmap_alloc_at(PhysicalAddress start, PhysicalAddress end, MemoryType type) {
    auto& count = mmap_count;

    for (size_t i = 0; i < count; i++) {
        auto& entry = mmap_entries[i];

        PhysicalAddress base = entry.base;
        PhysicalAddress top = entry.base + entry.length;

        if (start < base || start >= top || end > top) {
            continue;
        }

        if (base >= start && end < top && end > base) {
            entry.base += end - base;
            entry.length -= end - base;
            
            continue;
        }

        if (start > base && start < top && end >= top) {
            entry.length -= top - start;
            continue;
        }

        if (start > base && end < top) {
            entry.length -= top - start;

            mmap_entries[count] = { start, top - start, entry.type };
            count++;

            continue;
        }
    }

    mmap_entries[count] = { start, end - start, type };
    count++;
}

static void mmap_sort() {
    for (size_t i = 0; i < mmap_count; i++) {
        for (size_t j = i + 1; j < mmap_count; j++) {
            if (mmap_entries[i].base > mmap_entries[j].base) {
                std::swap(mmap_entries[i], mmap_entries[j]);
            }
        }
    }
}

extern "C" void init(multiboot_info* info) {
    if (!info) {
        return;
    }

    u32 modules = info->mods_count;
    if (modules < 1) {
        abort("No kernel module found. Halting.");
    }

    multiboot_module_t* kernel_module = reinterpret_cast<multiboot_module_t*>(info->mods_addr);
    u8* kernel_image = reinterpret_cast<u8*>(kernel_module->mod_start);

    Elf64_Ehdr* header = reinterpret_cast<Elf64_Ehdr*>(kernel_image);
    Elf64_Phdr kernel_program_headers[16];

    std::memcpy(kernel_program_headers, kernel_image + header->e_phoff, sizeof(Elf64_Phdr) * header->e_phnum);

    VirtualAddress kernel_virtual_base = 0xffffffff80000000;
    PhysicalAddress kernel_physical_base = 0x200000; // TODO: Randomize this

    VirtualAddress kernel_virtual_end = kernel_virtual_base;
    for (auto& ph : kernel_program_headers) {
        if (ph.p_type != PT_LOAD) {
            continue;
        }

        if (ph.p_vaddr < kernel_virtual_base) {
            abort("Kernel program header is below kernel virtual base. Halting.");
        }

        VirtualAddress end = ph.p_vaddr + ph.p_memsz;
        if (end > kernel_virtual_end) {
            kernel_virtual_end = end;
        }
    }
    
    kernel_virtual_end = std::align_up(kernel_virtual_end, 2 * MB);
    PhysicalAddress kernel_physical_end = kernel_physical_base + (kernel_virtual_end - kernel_virtual_base);

    u32 pml4 = ((kernel_virtual_base >> 39) & 0x1ff);
    boot_pml4t[pml4] = reinterpret_cast<u64>(kernel_pdpt) | 0x3;

    for (size_t i = 0; i < 2; i++) {
        u32 pdpt = ((kernel_virtual_base + i * GB) >> 30) & 0x1ff;
        kernel_pdpt[pdpt] = reinterpret_cast<u64>(kernel_pd[i]) | 0x3;
    }

    for (VirtualAddress address = kernel_virtual_base; address < kernel_virtual_end; address += 2 * MB) {
        PhysicalAddress physical = kernel_physical_base + (address - kernel_virtual_base);

        u32 pdpt = (address >> 30) & 0x1ff;
        u32 pdt = (address >> 21) & 0x1ff;

        kernel_pd[pdpt - 0x1fe][pdt] = physical | 0x83;
    }

    for (auto& ph : kernel_program_headers) {
        if (ph.p_type != PT_LOAD) {
            continue;
        }

        VirtualAddress vaddr = ph.p_vaddr;
        PhysicalAddress paddr = ph.p_offset + reinterpret_cast<PhysicalAddress>(kernel_image);

        size_t size = ph.p_filesz;

        std::memcpy(reinterpret_cast<void*>(vaddr), reinterpret_cast<void*>(paddr), size);
        std::memset(reinterpret_cast<void*>(vaddr + size), 0, ph.p_memsz - size);
    }

    BootInfo boot_info;

    boot_info.kernel_virtual_base = kernel_virtual_base;
    boot_info.kernel_physical_base = kernel_physical_base;
    boot_info.kernel_size = kernel_virtual_end - kernel_virtual_base;
    boot_info.kernel_heap_base = kernel_virtual_base + 1 * GB;
    boot_info.pml4t = boot_pml4t;
    boot_info.hhdm = 0xffff800000000000;
    boot_info.cmdline = reinterpret_cast<char*>(info->cmdline + boot_info.hhdm);

    std::memset(mmap_entries, 0, sizeof(MemoryMapEntry) * 256);

    PhysicalAddress loader_start = reinterpret_cast<PhysicalAddress>(&_loader_start);
    PhysicalAddress loader_end = reinterpret_cast<PhysicalAddress>(&_loader_end);

    for (size_t i = 0; i < info->mmap_length; i++) {
        auto* entry = reinterpret_cast<multiboot_memory_map_t*>(info->mmap_addr + i * sizeof(multiboot_memory_map_t));
        if (!entry->len || !entry->addr) {
            continue;
        }

        mmap_entries[mmap_count] = { entry->addr, entry->len, static_cast<MemoryType>(entry->type) };
        mmap_count++;
    }

    mmap_alloc_at(loader_start, loader_end, MemoryType::BootloaderReclaimable);
    mmap_alloc_at(kernel_physical_base, kernel_physical_end, MemoryType::KernelAndModules);

    mmap_sort();

    boot_info.mmap.entries = mmap_entries;
    boot_info.mmap.count = mmap_count - 1;

    void (*entry)(BootInfo const&) = reinterpret_cast<void(*)(BootInfo const&)>(header->e_entry);
    entry(boot_info);

    __builtin_unreachable();
}