#pragma once

#include <stdint.h>
#include <stddef.h>

#include <std/enums.h>
#include <std/types.h>

#define PACKED __attribute__((packed))
#define INTERRUPT [[gnu::interrupt]]
#define ALIGNED(x) __attribute__((aligned(x)))
#define ALWAYS_INLINE [[gnu::always_inline]]

constexpr u32 KERNEL_VIRTUAL_BASE = 0xC0000000;
constexpr u32 KERNEL_HEAP_ADDRESS = 0xE0000000; // Could this be replaced with the address of _kernel_end?

constexpr u32 PHYSICAL_VGA_ADDRESS = 0xB8000;
constexpr u32 VIRTUAL_VGA_ADDRESS = KERNEL_VIRTUAL_BASE + PHYSICAL_VGA_ADDRESS;

constexpr u32 PAGE_SIZE = 0x1000;
constexpr u32 SECTOR_SIZE = 0x200;

constexpr u32 INITIAL_KERNEL_HEAP_SIZE = 0x400000; // 1 MiB

using VirtualAddress = uintptr_t;
using PhysicalAddress = uintptr_t;

namespace kernel {

constexpr u32 page_base_of(u32 address) {
    return address & ~(PAGE_SIZE - 1);
}

constexpr u32 offset_in_page(u32 address) {
    return address & 0xFFF;
}

}