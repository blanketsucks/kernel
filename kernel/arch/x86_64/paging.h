#pragma once

#include <kernel/common.h>

namespace kernel::arch {

constexpr u32 get_page_directory_table_index(uintptr_t address) {
    return (address >> 30) & 0x1FF;
}

constexpr u32 get_page_directory_index(uintptr_t address) {
    return (address >> 21) & 0x1FF;
}

constexpr u32 get_page_table_index(uintptr_t address) {
    return (address >> 12) & 0x1FF;
}

}