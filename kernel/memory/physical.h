#pragma once

#include <kernel/arch/boot_info.h>
#include <kernel/common.h>

#include <std/stack.h>
#include <std/result.h>

namespace kernel::memory {

class PhysicalMemoryManager {
public:
    static void init(arch::BootInfo const&);

    static PhysicalMemoryManager* instance();

    size_t total_usable_memory() const { return m_total_usable_memory; }
    size_t used_memory() const { return m_allocations * PAGE_SIZE; }

    std::Stack<uintptr_t> const& physical_frames() const { return m_physical_frames; }

    size_t allocations() const { return m_allocations; }
    bool is_initialized() const { return m_initialized; }

    bool is_allocated(void* frame) const;

    [[nodiscard]] void* allocate();
    ErrorOr<void> free(void* frame);
private:
    bool m_initialized = false;
    size_t m_allocations = 0;

    size_t m_total_usable_memory = 0;

    std::Stack<uintptr_t> m_physical_frames;
};

}