#pragma once

#include <stdint.h>
#include <stddef.h>

#include <std/enums.h>
#include <std/types.h>

#define PACKED __attribute__((packed))
#define INTERRUPT [[gnu::interrupt]]
#define ALIGNED(x) __attribute__((aligned(x)))
#define ALWAYS_INLINE [[gnu::always_inline]]

namespace kernel {

using FlatPtr = uintptr_t;

using VirtualAddress = uintptr_t;
using PhysicalAddress = uintptr_t;
    
constexpr VirtualAddress MAX_VIRTUAL_ADDRESS = ~(VirtualAddress)0;

constexpr PhysicalAddress PHYSICAL_VGA_ADDRESS = 0xB8000;

constexpr size_t PAGE_SIZE = 0x1000;
constexpr size_t SECTOR_SIZE = 0x200;

constexpr size_t INITIAL_KERNEL_HEAP_SIZE = 0x400000; // 1 MiB

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