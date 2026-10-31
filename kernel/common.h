#pragma once

#include <stdint.h>
#include <stddef.h>

#include <std/enums.h>
#include <std/types.h>

#include <kernel/memory/virtual_address.h>
#include <kernel/memory/physical_address.h>

#define PACKED __attribute__((packed))
#define INTERRUPT [[gnu::interrupt]]
#define ALIGNED(x) __attribute__((aligned(x)))
#define ALWAYS_INLINE [[gnu::always_inline]]

#define NO_COPY(Class)                                  \
    Class(const Class&) = delete;                       \
    Class& operator=(const Class&) = delete;

#define NO_MOVE(Class)                                  \
    Class(Class&&) = delete;                            \
    Class& operator=(Class&&) = delete;

#define DEFAULT_COPY(Class)                             \
    Class(const Class&) = default;                      \
    Class& operator=(const Class&) = default;

#define DEFAULT_MOVE(Class)                             \
    Class(Class&&) = default;                           \
    Class& operator=(Class&&) = default;

namespace kernel {

constexpr size_t PAGE_SIZE = 0x1000;
constexpr size_t SECTOR_SIZE = 0x200;

constexpr size_t KiB = 1024;
constexpr size_t MiB = 1024 * KiB;
constexpr size_t GiB = 1024 * MiB;

constexpr size_t INITIAL_KERNEL_HEAP_SIZE = 4 * MiB;

static constexpr FlatPtr page_base_of(FlatPtr address) {
    return address & ~(PAGE_SIZE - 1);
}

static constexpr FlatPtr offset_in_page(FlatPtr address) {
    return address & (PAGE_SIZE - 1);
}

}