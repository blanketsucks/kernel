#pragma once

#include <kernel/common.h>

namespace kernel::arch {

enum class MemoryType {
    Available = 1,
    Reserved = 2,
    ACPI = 3,
    NVS = 4,
    BadMemory = 5,
};

struct MemoryMapEntry {
    u64 base;
    u64 length;
    MemoryType type;
} PACKED;

struct MemoryMap {
    u64 count;
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
    size_t kernel_size;
    
    MemoryMap mmap;
    FramebufferInfo framebuffer;
} PACKED;

}