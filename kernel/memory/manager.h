#pragma once

#include <kernel/common.h>
#include <kernel/memory/region.h>
#include <kernel/sync/spinlock.h>

#include <kernel/boot/boot_info.h>
#include <kernel/arch/page_directory.h>
#include <kernel/arch/registers.h>

#include <std/result.h>

#define MM kernel::MemoryManager::instance()

namespace kernel {

namespace memory {
    class PhysicalMemoryManager;
    class PageDirectory;
}

union PageFault {
    // Comments taken from Intel's manual (Vol. 3A 4-37)
    struct {
        // 0 = The fault was caused by a non-present page.
        // 1 = The fault was caused by a page-level protection violation.
        u8 present: 1;

        // 0 = The access causing the fault was a read.
        // 1 = The access causing the fault was a write.
        u8 rw : 1;

        // 0 = The access causing the fault originated when the processor was executing in supervisor mode.
        // 1 = The access causing the fault originated when the processor was executing in user mode.
        u8 user : 1;

        // 0 = The fault was not caused by reserved bit violation.
        // 1 = The fault was caused by reserved bits set to 1 in some paging-structure entry.
        u8 rsvd : 1;

        // 0 = The fault was not caused by an instruction fetch.
        // 1 = The fault was caused by an instruction fetch.
        u8 id : 1;

        // 0 = The fault was not caused protection keys.
        // 1 = There was a protection key violation.
        u8 pk : 1;

        // 0 = The fault was not caused by SGX.
        // 1 = There was an SGX access-control violation.
        u8 ss : 1;

        // 0 = The fault occurred during ordinary paging or due to access rights.
        // 1 = The fault occurred during HLAT paging.
        u8 hlt : 1;

        u8 reserved0 : 7;
        
        // 0 = The fault was not caused by SGX.
        // 1 = There was an SGX access-control violation.
        u8 sgx : 1;

        u16 reserved1;
    } PACKED;

    u32 value;

    PageFault(u32 value) : value(value) {}
};

struct PhysicalPage {
    enum Flags : u16 {
        CoW = 1 << 0,
    };

    u16 ref_count = 0;
    u16 flags = 0;
};

class MemoryManager {
public:
    MemoryManager();

    static void init();
    static MemoryManager* instance();

    static void page_fault_handler(arch::InterruptRegisters*);

    static arch::PageDirectory* kernel_page_directory();

    static size_t current_kernel_heap_offset();

    static StringView get_fault_message(PageFault fault, memory::Region* region);

    memory::RegionAllocator& heap_region_allocator() { return *m_heap_region_allocator; }
    memory::RegionAllocator& kernel_region_allocator() { return *m_kernel_region_allocator; }

    bool is_mapped(void* addr);
    PhysicalAddress get_physical_address(void* addr);

    ErrorOr<void*> allocate_page_frame();
    ErrorOr<void*> allocate_contiguous_frames(size_t count);

    ErrorOr<void> free_page_frame(void* frame);

    ErrorOr<void*> allocate(memory::RegionAllocator&, size_t size, PageFlags flags);
    ErrorOr<void*> allocate_at(memory::RegionAllocator&, VirtualAddress address, size_t size, PageFlags flags);

    ErrorOr<void> map_region(arch::PageDirectory*, memory::Region*, PageFlags flags);

    ErrorOr<void> free(memory::RegionAllocator&, void* ptr, size_t size);
    ErrorOr<void> free(arch::PageDirectory*, VirtualAddress address, size_t size);

    ErrorOr<void*> allocate_heap_region(size_t size);
    ErrorOr<void> free_heap_region(void* ptr, size_t size);

    ErrorOr<void*> allocate_kernel_region(size_t size);
    ErrorOr<void> free_kernel_region(void* ptr, size_t size);

    ErrorOr<void*> allocate_dma_region(size_t size);
    ErrorOr<void> free_dma_region(void* ptr, size_t size);

    // Map an already existing physical region into the kernel's address space
    void* map_physical_region(void* ptr, size_t size);

    void unmap_kernel_region(void* ptr);

    void* map_from_page_directory(arch::PageDirectory*, void* ptr, size_t size);

    void copy_physical_memory(void* dst, void* src, size_t size);

    PhysicalPage* get_physical_page(PhysicalAddress address);

    SpinLock& liballoc_lock() { return m_liballoc_lock; }
    
private:
    void initialize();
    void create_physical_pages();

    bool try_allocate_contiguous(arch::PageDirectory*, memory::Region*, PageFlags flags);

    memory::PhysicalMemoryManager* m_pmm;

    RefPtr<memory::RegionAllocator> m_heap_region_allocator;
    RefPtr<memory::RegionAllocator> m_kernel_region_allocator;

    PhysicalPage* m_physical_pages = nullptr;
    size_t m_physical_pages_count = 0;

    SpinLock m_liballoc_lock;
    SpinLock m_lock;
};

class TemporaryMapping {
public:
    TemporaryMapping(arch::PageDirectory&, void* ptr, size_t size);
    ~TemporaryMapping();

    u8* ptr() const { return m_ptr; }
    size_t size() const { return m_size; }

private:
    u8* m_ptr;
    size_t m_size;
};

}

// We have to place this definition here to avoid circular dependencies
template<> struct std::Formatter<kernel::VirtualAddress> {
    static void format(FormatBuffer& buffer, const kernel::VirtualAddress& value, const FormatStyle& style) {
        std::Formatter<FlatPtr>::format(buffer, static_cast<FlatPtr>(value), style);
    }
};

int liballoc_lock();
int liballoc_unlock();

void* liballoc_alloc(size_t pages);
int liballoc_free(void* address, size_t pages);
