#pragma once

#include <kernel/common.h>

#include <std/stack.h>
#include <std/result.h>

namespace kernel::memory {

class PhysicalMemoryManager {
public:
    static void init(multiboot_info_t* header);

    static PhysicalMemoryManager* instance();

    u32 total_usable_ram() const { return m_total_usable_ram; }

    Stack<u32> const& physical_frames() const { return m_physical_frames; }

    u32 allocations() const { return m_allocations; }
    bool is_initialized() const { return m_initialized; }

    bool is_allocated(void* frame) const;

    [[nodiscard]] ErrorOr<void*> allocate();
    [[nodiscard]] ErrorOr<void> free(void* frame);
private:
    bool m_initialized = false;
    u32 m_allocations = 0;

    u32 m_total_usable_ram = 0;

    Stack<u32> m_physical_frames;
};

}