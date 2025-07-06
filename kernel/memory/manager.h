#pragma once

#include <kernel/common.h>
#include <kernel/memory/region.h>
#include <kernel/sync/spinlock.h>

#include <kernel/boot/boot_info.h>
#include <kernel/arch/page_directory.h>
#include <kernel/arch/registers.h>

#include <std/result.h>

#define MM kernel::memory::MemoryManager::instance()

namespace kernel::memory {

class PhysicalMemoryManager;
class PageDirectory;

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
        u8 id : 1; // 0 = instruction fetch, 1 = data access

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

    RegionAllocator& heap_region_allocator() { return *m_heap_region_allocator; }
    RegionAllocator& kernel_region_allocator() { return *m_kernel_region_allocator; }

    bool is_mapped(void* addr);
    PhysicalAddress get_physical_address(void* addr);

    [[nodiscard]] void* allocate_page_frame();
    [[nodiscard]] void* allocate_contiguous_frames(size_t count);

    ErrorOr<void> free_page_frame(void* frame);
    
    void* allocate(RegionAllocator&, size_t size, PageFlags flags);
    void* allocate_at(RegionAllocator&, VirtualAddress address, size_t size, PageFlags flags);
    
    ErrorOr<void> map_region(arch::PageDirectory*, Region*, PageFlags flags);

    ErrorOr<void> free(RegionAllocator&, Region*);
    ErrorOr<void> free(RegionAllocator&, void* ptr, size_t size);

    [[nodiscard]] void* allocate_heap_region(size_t size);
    ErrorOr<void> free_heap_region(void* ptr, size_t size);

    [[nodiscard]] void* allocate_kernel_region(size_t size);
    ErrorOr<void> free_kernel_region(void* ptr, size_t size);

    [[nodiscard]] void* allocate_dma_region(size_t size);
    ErrorOr<void> free_dma_region(void* ptr, size_t size);

    // Map an already existing physical region into the kernel's address space
    void* map_physical_region(void* ptr, size_t size);
    void unmap_physical_region(void* ptr);

    void* map_from_page_directory(arch::PageDirectory*, void* ptr, size_t size);

    void copy_physical_memory(void* dst, void* src, size_t size);

    PhysicalPage* get_physical_page(PhysicalAddress address);

    SpinLock& liballoc_lock() { return m_liballoc_lock; }
    
private:
    void initialize();
    void create_physical_pages();

    bool try_allocate_contiguous(arch::PageDirectory*, Region*, PageFlags flags);

    PhysicalMemoryManager* m_pmm;

    RefPtr<RegionAllocator> m_heap_region_allocator;
    RefPtr<RegionAllocator> m_kernel_region_allocator;

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

// The following is from liballoc

/** This function is supposed to lock the memory data structures. It
 * could be as simple as disabling interrupts or acquiring a spinlock.
 * It's up to you to decide. 
 *
 * \return 0 if the lock was acquired successfully. Anything else is
 * failure.
 */
int liballoc_lock();

/** This function unlocks what was previously locked by the liballoc_lock
 * function.  If it disabled interrupts, it enables interrupts. If it
 * had acquiried a spinlock, it releases the spinlock. etc.
 *
 * \return 0 if the lock was successfully released.
 */
int liballoc_unlock();

/** This is the hook into the local system which allocates pages. It
 * accepts an integer parameter which is the number of pages
 * required.  The page size was set up in the liballoc_init function.
 *
 * \return NULL if the pages were not allocated.
 * \return A pointer to the allocated memory.
 */
void* liballoc_alloc(size_t pages);

/** This frees previously allocated memory. The void* parameter passed
 * to the function is the exact same value returned from a previous
 * liballoc_alloc call.
 *
 * The integer value is the number of pages to free.
 *
 * \return 0 if the memory was successfully freed.
 */
int liballoc_free(void* address, size_t pages);
