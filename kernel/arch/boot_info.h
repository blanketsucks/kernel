#pragma once

#include <kernel/common.h>
#include <std/string_view.h>
#include <limine.h>

namespace kernel::arch {

enum class MemoryType {
    Available = 1,
    Reserved = 2,
    ACPI = 3,
    NVS = 4,
    BadMemory = 5,
    BootloaderReclaimable = 6,
    KernelAndModules = 7,
    Framebuffer = 8
};

struct MemoryMapEntry {
    u64 base;
    u64 length;
    MemoryType type;
} PACKED;

struct MemoryMap {
    size_t count;
    MemoryMapEntry* entries;
} PACKED;

struct FramebufferInfo {
    u64 width;
    u64 height;
    u64 pitch;
    u16 bpp;
    void* address;

    u64 edid_size;
    void* edid;
} PACKED;

struct BootInfo {
    u64 kernel_virtual_base;
    u64 kernel_physical_base;

    u64 kernel_heap_base;

    // Page aligned kernel size
    size_t kernel_size;

    char* cmdline;

    u64 hhdm;

    void* rsdp = nullptr;
    
    MemoryMap mmap;
    FramebufferInfo framebuffer;
} PACKED;


constexpr std::StringView memory_type_to_string(MemoryType type) {
    switch (type) {
        case MemoryType::Available: return "Available";
        case MemoryType::Reserved: return "Reserved";
        case MemoryType::ACPI: return "ACPI";
        case MemoryType::NVS: return "NVS";
        case MemoryType::BadMemory: return "Bad memory";
        case MemoryType::BootloaderReclaimable: return "Bootloader reclaimable";
        case MemoryType::KernelAndModules: return "Kernel and modules";
        case MemoryType::Framebuffer: return "Framebuffer";
    }

    return "Unknown";
}

}

extern kernel::arch::BootInfo const* g_boot_info;