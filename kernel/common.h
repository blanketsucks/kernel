#pragma once

#include <stdint.h>
#include <stddef.h>

#include <std/enums.h>
#include <std/types.h>

#define PACKED __attribute__((packed))
#define INTERRUPT [[gnu::interrupt]]
#define ALIGNED(x) __attribute__((aligned(x)))
#define ALWAYS_INLINE [[gnu::always_inline]]

using VirtualAddress = uintptr_t;
using PhysicalAddress = uintptr_t;

constexpr VirtualAddress KERNEL_VIRTUAL_BASE = 0xC0000000;
constexpr VirtualAddress KERNEL_HEAP_ADDRESS = 0xE0000000; // Could this be replaced with the address of _kernel_end?

constexpr PhysicalAddress PHYSICAL_VGA_ADDRESS = 0xB8000;
constexpr VirtualAddress VIRTUAL_VGA_ADDRESS = KERNEL_VIRTUAL_BASE + PHYSICAL_VGA_ADDRESS;

constexpr size_t PAGE_SIZE = 0x1000;
constexpr size_t SECTOR_SIZE = 0x200;

constexpr size_t INITIAL_KERNEL_HEAP_SIZE = 0x400000; // 1 MiB

namespace kernel {

constexpr size_t KB = 1024;
constexpr size_t MB = 1024 * KB;
constexpr size_t GB = 1024 * MB;

constexpr VirtualAddress page_base_of(VirtualAddress address) {
    return address & ~(PAGE_SIZE - 1);
}

constexpr VirtualAddress offset_in_page(VirtualAddress address) {
    return address & 0xFFF;
}

}