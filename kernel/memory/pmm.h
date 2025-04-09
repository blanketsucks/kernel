#pragma once

#include <kernel/boot/boot_info.h>
#include <kernel/common.h>

#include <std/stack.h>
#include <std/result.h>
#include <std/bitmap.h>

namespace kernel::memory {

class PhysicalRegion {
public:
    PhysicalRegion(PhysicalAddress base, size_t size);

    PhysicalAddress base() const { return m_base; }
    size_t size() const { return m_size; }

    size_t page_count() const { return m_size / PAGE_SIZE; }
    size_t free_pages() const { return m_free_pages; }

    bool usable() const { return m_free_pages > 0; }

    bool contains(PhysicalAddress address) const {
        return address >= m_base && address < m_base + m_size;
    }

    [[nodiscard]] ErrorOr<void*> allocate();
    [[nodiscard]] ErrorOr<void*> allocate_contiguous(size_t count);

    ErrorOr<void> free(void* frame, size_t count);
    
private:
    PhysicalAddress m_base;
    size_t m_size;

    std::Bitmap m_bitmap;
    size_t m_free_pages = 0;

    friend class PhysicalMemoryManager;
};

class PhysicalMemoryManager {
public:
    static constexpr size_t PREFRAME_COUNT = 1024;

    static PhysicalMemoryManager* create(BootInfo const&);

    size_t total_usable_memory() const { return m_total_usable_memory; }
    size_t total_pages() const { return m_total_pages; }
    
    size_t used_memory() const { return m_allocations * PAGE_SIZE; }

    size_t allocations() const { return m_allocations; }
    bool is_initialized() const { return m_initialized; }

    [[nodiscard]] ErrorOr<void*> allocate();
    [[nodiscard]] ErrorOr<void*> allocate_contiguous(size_t count);

    ErrorOr<void> free(void* frame, size_t count);

    void fill_preframe_buffer();

private:
    void init(BootInfo const&);

    bool m_initialized = false;
    size_t m_allocations = 0;

    size_t m_total_usable_memory = 0;
    size_t m_total_pages = 0;

    Vector<PhysicalRegion> m_physical_regions;
    std::Stack<PhysicalAddress> m_frames;
};

}