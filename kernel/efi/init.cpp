#include <kernel/efi/uefi.h>
#include <kernel/boot/boot_info.h>

#include <std/utility.h>
#include <std/elf.h>
#include <std/cstring.h>

#define GET_PAGE(x, index) reinterpret_cast<u64*>(x + (index * PAGE_SIZE))

static EFISystemTable* ST = nullptr;

static void println(const char16_t* str) {
    if (!ST) {
        return;
    }

    EFISimpleTextOutputProtocol* stdout = ST->stdout;

    stdout->OutputString(stdout, str);
    stdout->OutputString(stdout, u"\r\n");
}

struct EFIMemoryMap {
    EFIMemoryDescriptor descriptors[512]; // FIXME: Dynamically allocate the descriptors based on the count returned by GetMemoryMap
    size_t count;
    size_t descriptor_size;
};

struct MemoryMap {
    kernel::MemoryMapEntry entries[512];
    size_t count;
};

static EFIMemoryMap s_efi_mmap;
static MemoryMap s_mmap;

static constexpr char16_t DEFAULT_KERNEL_PATH[] = u"\\kernel";

static EFIStatus parse_memory_map() {
    u64 map_key = 0;
    u64 count = sizeof(EFIMemoryMap::descriptors);
    u32 descriptor_version = 0;

    EFIStatus status = ST->boot_services->GetMemoryMap(&count, s_efi_mmap.descriptors, &map_key, &s_efi_mmap.descriptor_size, &descriptor_version);
    if (EFI_ERROR(status)) {
        println(u"Failed get the memory map");
        return status;
    }

    s_efi_mmap.count = count / s_efi_mmap.descriptor_size;
    return EFI_SUCCESS;
}

static inline EFIMemoryDescriptor* get_memory_descriptor(size_t index) {
    if (index >= s_efi_mmap.count) {
        return nullptr;
    }

    return reinterpret_cast<EFIMemoryDescriptor*>((u8*)s_efi_mmap.descriptors + index * s_efi_mmap.descriptor_size);
}

extern EFIStatus efi_main(EFIHandle image_handle, EFISystemTable* sys_table) {
    using namespace kernel;

    ST = sys_table;

    EFIGUID loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFILoadedImageProtocol* lip = nullptr;
    
    EFIStatus status = sys_table->boot_services->OpenProtocol(
        image_handle,
        &loaded_image_guid,
        reinterpret_cast<void**>(&lip),
        image_handle,
        nullptr,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );

    if (EFI_ERROR(status)) {
        println(u"Failed to open Loaded Image Protocol");
        return status;
    }

    EFIGUID simple_file_system_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFISimpleFileSystemProtocol* sfs = nullptr;

    status = sys_table->boot_services->OpenProtocol(
        lip->device_handle,
        &simple_file_system_guid,
        reinterpret_cast<void**>(&sfs),
        lip->device_handle,
        nullptr,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );

    if (EFI_ERROR(status)) {
        println(u"Failed to open Simple File System Protocol");
        return status;
    }

    EFIFileProtocol* root = nullptr;
    status = sfs->OpenVolume(sfs, &root);
    
    if (EFI_ERROR(status)) {
        println(u"Failed to open root volume");
        return status;
    }

    EFIFileProtocol* file = nullptr;
    status = root->Open(root, &file, DEFAULT_KERNEL_PATH, EFI_FILE_MODE_READ, 0);

    if (EFI_ERROR(status)) {
        println(u"Failed to open kernel file");
        return status;
    }

    u8 buffer[sizeof(EFIFileInfo) + sizeof(DEFAULT_KERNEL_PATH)];
    EFIGUID information_type = EFI_FILE_INFO_ID;

    u64 buffer_size = sizeof(buffer);
    status = file->GetInfo(file, &information_type, &buffer_size, &buffer);

    if (EFI_ERROR(status)) {
        println(u"Failed to get kernel file information");
        return status;
    }
    
    EFIFileInfo* file_info = reinterpret_cast<EFIFileInfo*>(buffer);

    size_t unaligned_size = file_info->file_size;
    size_t size = std::align_up(unaligned_size, PAGE_SIZE);

    size_t pages = size / PAGE_SIZE;

    uintptr_t address = 0;
    status = sys_table->boot_services->AllocatePages(AllocateAnyPages, EfiLoaderData, pages, &address);

    if (EFI_ERROR(status)) {
        println(u"Failed to allocate memory for kernel file");
        return status;
    }

    status = file->Read(file, &unaligned_size, reinterpret_cast<void*>(address));
    if (EFI_ERROR(status)) {
        println(u"Failed to read kernel file");
        sys_table->boot_services->FreePages(address, pages);

        return status;
    }

    u8* kernel_image = reinterpret_cast<u8*>(address);

    Elf64_Ehdr* header = reinterpret_cast<Elf64_Ehdr*>(kernel_image);
    Elf64_Phdr kernel_program_headers[16];

    if (header->e_phnum > 16) {
        println(u"Kernel has too many program headers");
        sys_table->boot_services->FreePages(address, pages);

        return EFI_LOAD_ERROR;
    }

    uintptr_t memory = 0;
    // Allocate 9 pages for the kernel.
    //   1 page for the PML4T.
    //   1 page for the identity-mapped PDPT and the accompanying 4 PDs (Identity mapping the first 4GB of memory).
    //   1 page for the kernel PDPT and the accompanying 2 PDs (The kernel occupies the upper 2GB of virtual memory).
    status = sys_table->boot_services->AllocatePages(AllocateAnyPages, EfiLoaderData, 1 + 1 + 4 + 1 + 2, &memory);

    if (EFI_ERROR(status)) {
        println(u"Failed to allocate memory for kernel page tables");
        sys_table->boot_services->FreePages(address, pages);

        return status;
    }

    u64* pml4t = reinterpret_cast<u64*>(memory);
    std::memset(pml4t, 0, PAGE_SIZE * 9); // Clear everything

    u64* pdpt = GET_PAGE(memory, 1);
    u64* pd[4] = { GET_PAGE(memory, 2), GET_PAGE(memory, 3), GET_PAGE(memory, 4), GET_PAGE(memory, 5) };

    pml4t[0] = reinterpret_cast<u64>(pdpt) | 0x3;
    for (size_t i = 0; i < 4; i++) {
        pdpt[i] = reinterpret_cast<u64>(pd[i]) | 0x3;
    }

    for (size_t i = 0; i < 4; i++) {
        for (size_t j = 0; j < 512; j++) {
            pd[i][j] = (i * GB + j * (2 * MB)) | 0x83;
        }
    }

    u64* kernel_pdpt = GET_PAGE(memory, 6);
    u64* kernel_pd[2] = { GET_PAGE(memory, 7), GET_PAGE(memory, 8) };

    std::memcpy(kernel_program_headers, kernel_image + header->e_phoff, header->e_phnum * sizeof(Elf64_Phdr));

    VirtualAddress kernel_virtual_base = 0xffffffff80000000;
    VirtualAddress kernel_virtual_end = kernel_virtual_base;

    for (size_t i = 0; i < header->e_phnum; i++) {
        auto& ph = kernel_program_headers[i];
        if (ph.p_type != PT_LOAD) {
            continue;
        }

        VirtualAddress end = ph.p_vaddr + ph.p_memsz;
        if (end > kernel_virtual_end) {
            kernel_virtual_end = end;
        }
    }

    kernel_virtual_end = std::align_up(kernel_virtual_end, 2 * MB);

    size_t kernel_size = kernel_virtual_end - kernel_virtual_base;
    size_t kernel_pages = kernel_size / PAGE_SIZE;

    u32 pml4e = ((kernel_virtual_base >> 39) & 0x1ff);
    pml4t[pml4e] = reinterpret_cast<u64>(kernel_pdpt) | 0x3;

    for (size_t i = 0; i < 2; i++) {
        u32 pdpte = ((kernel_virtual_base + i * GB) >> 30) & 0x1ff;
        kernel_pdpt[pdpte] = reinterpret_cast<u64>(kernel_pd[i]) | 0x3;
    }

    status = parse_memory_map();
    if (EFI_ERROR(status)) {
        sys_table->boot_services->FreePages(memory, 9);
        sys_table->boot_services->FreePages(address, pages);

        return status;
    }

    // We can't use AllocatePages to allocate memory directly since that only gives us 4Kb aligned pages while we need
    // 2Mb aligned pages.
    PhysicalAddress kernel_physical_base = 0;
    for (size_t i = 0; i < s_efi_mmap.count; i++) {
        auto* descriptor = get_memory_descriptor(i);
        if (descriptor->type != EfiConventionalMemory && descriptor->type != EfiLoaderData) {
            continue;
        }

        if (descriptor->number_of_pages < kernel_pages) {
            continue;
        }

        if (descriptor->physical_start % (2 * MB)) {
            size_t aligned = std::align_up(descriptor->physical_start, 2 * MB);
            size_t offset = aligned - descriptor->physical_start;

            if (descriptor->number_of_pages < (kernel_pages + offset / PAGE_SIZE)) {
                continue;
            }
        
            kernel_physical_base = aligned;
            EFIStatus status = sys_table->boot_services->AllocatePages(
                AllocateAddress,
                EfiLoaderData,
                kernel_pages,
                &kernel_physical_base
            );

            if (EFI_ERROR(status)) {
                continue;
            }

            break;
        }

        kernel_physical_base = descriptor->physical_start;
        EFIStatus status = sys_table->boot_services->AllocatePages(
            AllocateAddress,
            EfiLoaderData,
            kernel_pages,
            &kernel_physical_base
        );

        if (EFI_ERROR(status)) {
            continue;
        }

        break;
    }

    if (!kernel_physical_base) {
        println(u"Failed to find a suitable physical address for the kernel");
        sys_table->boot_services->FreePages(memory, 9);
        sys_table->boot_services->FreePages(address, pages);
        
        return EFI_LOAD_ERROR;
    }
    
    parse_memory_map();
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

        PhysicalAddress dst = kernel_physical_base + (ph.p_vaddr - kernel_virtual_base);
        PhysicalAddress src = ph.p_offset + reinterpret_cast<PhysicalAddress>(kernel_image);

        size_t size = ph.p_filesz;

        std::memcpy(reinterpret_cast<void*>(dst), reinterpret_cast<void*>(src), size);
        std::memset(reinterpret_cast<void*>(dst + size), 0, ph.p_memsz - size);
    }
    
    asm volatile("mov %0, %%cr3" :: "r"(pml4t));

    BootInfo boot_info;
    std::memset(&boot_info, 0, sizeof(BootInfo));

    boot_info.kernel_virtual_base = kernel_virtual_base;
    boot_info.kernel_physical_base = kernel_physical_base;
    boot_info.kernel_size = kernel_size;
    boot_info.kernel_heap_base = kernel_virtual_base + 1 * GB;

    std::memset(&s_mmap, 0, sizeof(::MemoryMap));

    boot_info.mmap.entries = s_mmap.entries;
    boot_info.mmap.count = s_mmap.count;

    void (*entry)(BootInfo const&) = reinterpret_cast<void(*)(BootInfo const&)>(header->e_entry);
    entry(boot_info);

    return EFI_SUCCESS;
}