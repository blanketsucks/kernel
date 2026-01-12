#pragma once

#include <stdint.h>
#include <stddef.h>

#include <std/enums.h>
#include <std/types.h>

#include <kernel/memory/virtual_address.h>

#define PACKED __attribute__((packed))
#define INTERRUPT [[gnu::interrupt]]
#define ALIGNED(x) __attribute__((aligned(x)))
#define ALWAYS_INLINE [[gnu::always_inline]]

namespace kernel {

using PhysicalAddress = uintptr_t;

constexpr PhysicalAddress PHYSICAL_VGA_ADDRESS = 0xB8000;

constexpr size_t PAGE_SIZE = 0x1000;
constexpr size_t SECTOR_SIZE = 0x200;

constexpr size_t KB = 1024;
constexpr size_t MB = 1024 * KB;
constexpr size_t GB = 1024 * MB;

constexpr size_t INITIAL_KERNEL_HEAP_SIZE = 4 * MB;

static constexpr FlatPtr page_base_of(FlatPtr address) {
    return address & ~(PAGE_SIZE - 1);
}

static constexpr FlatPtr offset_in_page(FlatPtr address) {
    return address & (PAGE_SIZE - 1);
}

}