#pragma once

#include <kernel/common.h>
#include <kernel/multiboot.h>
#include <std/result.h>

#include <kernel/memory/region.h>
#include <kernel/cpu/idt.h>
#include <kernel/sync/spinlock.h>

#define MM memory::MemoryManager::instance()

namespace kernel::memory {

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

class MemoryManager {
public:
    MemoryManager();

    static void init(multiboot_info_t* header);
    static MemoryManager* instance();

    static void page_fault_handler(cpu::Registers*);

    static PageDirectory* kernel_page_directory();

    Region& heap_region() { return m_heap_region; }
    Region& kernel_region() { return m_kernel_region; }

    bool is_mapped(void* addr);
    u32 get_physical_address(void* addr);

    [[nodiscard]] ErrorOr<void*> allocate_physical_frame();
    [[nodiscard]] ErrorOr<void> free_physical_frame(void* frame);

    [[nodiscard]] ErrorOr<void*> allocate_heap_region(size_t pages);
    [[nodiscard]] ErrorOr<void> free_heap_region(void* start, size_t pages);

    [[nodiscard]] ErrorOr<void*> allocate_kernel_region(size_t pages);
    [[nodiscard]] ErrorOr<void> free_kernel_region(void* start, size_t pages);

    // Allocates from the kernel region
    void* allocate(size_t size);
    void free(void* ptr);

    template<typename T> T* allocate() { return this->allocate(sizeof(T)); }

    // Map an already existing physical region into the kernel's address space
    void* map_physical_region(u32 start, size_t size);
    void unmap_physical_region(void* ptr);

    SpinLock& alloc_lock() { return m_alloc_lock; }
private:
    ErrorOr<void*> allocate_from_region(Region& region, size_t pages);
    ErrorOr<void> free_from_region(Region& region, void* start, size_t pages);

    Region m_heap_region;
    Region m_kernel_region;

    SpinLock m_alloc_lock;
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

void* operator new(size_t size);
inline void* operator new(size_t, void* p) { return p; }

void* operator new[](size_t size);
inline void* operator new[](size_t, void* p) { return p; }

void operator delete(void* p);
void operator delete[](void* p);

void operator delete(void* p, size_t size);
void operator delete[](void* p, size_t size);