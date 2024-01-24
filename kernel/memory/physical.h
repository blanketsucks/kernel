#pragma once

#include <kernel/common.h>

#include <std/stack.h>
#include <std/result.h>

namespace kernel::memory {

// Number to be used as a magic number for allocated frames
constexpr u32 FRAME_ALLOCATED = 0x1;

class PhysicalMemoryManager {
public:
    static void init(u32 mem_lower, u32 mem_upper);

    static PhysicalMemoryManager* instance();

    u32 upper_address() const { return m_upper_address; }
    Stack<u32> const& physical_frames() const { return m_physical_frames; }

    u32 allocations() const { return m_allocations; }
    bool is_initialized() const { return initialized; }

    bool is_allocated(void* frame) const;

    [[nodiscard]] ErrorOr<void*> allocate();
    [[nodiscard]] ErrorOr<void> free(void* frame);
private:
    bool initialized = false;
    u32 m_allocations = 0;

    u32 m_upper_address = 0;

    u32 frames = 0;
    Stack<u32> m_physical_frames;
};

}