#include <kernel/memory/buddy_allocator.h>

namespace kernel::memory {

BuddyAllocator::BuddyAllocator(PhysicalAddress base, size_t pages) : m_base_address(base), m_total_pages(pages) {
    

}

}