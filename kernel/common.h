#pragma once

#include <stdint.h>
#include <stddef.h>

#include <std/enums.h>

#define PACKED __attribute__((packed))
#define INTERRUPT [[gnu::interrupt]]
#define ALIGNED(x) __attribute__((aligned(x)))

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

constexpr u32 KERNEL_VIRTUAL_BASE = 0xC0000000;
constexpr u32 KERNEL_HEAP_ADDRESS = 0xE0000000; // Could this be replaced with the address of _kernel_end?

constexpr u32 PHYSICAL_VGA_ADDRESS = 0xB8000;
constexpr u32 VIRTUAL_VGA_ADDRESS = KERNEL_VIRTUAL_BASE + PHYSICAL_VGA_ADDRESS;

constexpr u32 PAGE_SIZE = 0x1000;
constexpr u32 SECTOR_SIZE = 0x200;

constexpr u32 INITIAL_KERNEL_HEAP_SIZE = 0x100000; // 1 MiB