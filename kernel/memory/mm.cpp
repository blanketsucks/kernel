#include <kernel/memory/mm.h>
#include <kernel/memory/paging.h>
#include <kernel/memory/physical.h>
#include <kernel/memory/liballoc.h>

#include <kernel/panic.h>
#include <kernel/vga.h>
#include <kernel/serial.h>

#include <std/cstring.h>
#include <std/utility.h>

namespace kernel::memory {

static u8 s_kernel_heap[INITIAL_KERNEL_HEAP_SIZE];
static u32 s_kernel_heap_offset = 0;

static MemoryManager s_memory_manager = MemoryManager();

MemoryManager::MemoryManager() {
    m_heap_region = Region(KERNEL_HEAP_ADDRESS, 0xFFFFFFFF - KERNEL_VIRTUAL_BASE);
    m_kernel_region = Region(KERNEL_VIRTUAL_BASE, KERNEL_HEAP_ADDRESS - KERNEL_VIRTUAL_BASE);
}

void MemoryManager::init(multiboot_info_t* header) {
    PhysicalMemoryManager::init(header);
    PageDirectory::init_kernel(s_memory_manager.m_kernel_region);
}

void MemoryManager::page_fault_handler(cpu::Registers* regs) {
    u32 address = 0;
    asm volatile("mov %%cr2, %0" : "=r"(address));

    serial::printf("\n\nPage fault (address 0x%x) at EIP=%#x:\n", address, regs->eip);
    PageFault fault = regs->errno;

    if (!fault.present) {
        serial::printf(" - Page not present\n");
    }

    if (fault.rw) {
        serial::printf(" - Write access\n");
    } else {
        serial::printf(" - Read access\n");
    }

    if (fault.user) {
        serial::printf(" - User mode\n\n");
    } else {
        serial::printf(" - Kernel mode\n\n");
    }

    kernel::panic("Page fault", false);
}

MemoryManager* MemoryManager::instance() {
    return &s_memory_manager;
}

PageDirectory* MemoryManager::kernel_page_directory() {
    return PageDirectory::kernel_page_directory();
}

ErrorOr<void*> MemoryManager::allocate_physical_frame() {
    auto* pmm = PhysicalMemoryManager::instance();
    return pmm->allocate();
}

ErrorOr<void> MemoryManager::free_physical_frame(void* frame) {
    auto* pmm = PhysicalMemoryManager::instance();
    return pmm->free(frame);
}

ErrorOr<void*> MemoryManager::allocate_from_region(Region& region, size_t pages) {
    auto dir = PageDirectory::kernel_page_directory();

    auto space = region.allocate(pages * PAGE_SIZE, Permissions::Read | Permissions::Write, true);
    if (!space) {
        return Error(ENOMEM);
    }

    for (size_t i = 0; i < pages; i++) {
        void* frame = TRY(this->allocate_physical_frame());
        dir->map(
            space->address() + i * PAGE_SIZE, 
            reinterpret_cast<PhysicalAddress>(frame), 
            PageFlags::Writable
        );
    }

    return reinterpret_cast<void*>(space->address());
}

ErrorOr<void> MemoryManager::free_from_region(Region& region, void* start, size_t pages) {
    if (!this->is_mapped(start)) return Error(EFAULT);

    auto space = region.find_space(reinterpret_cast<u32>(start));
    if (!space) return Error(EFAULT);

    auto dir = PageDirectory::kernel_page_directory();
    for (size_t i = 0; i < pages; i++) {
        u32 virt = space->address() + i * PAGE_SIZE;

        PhysicalAddress physical = dir->get_physical_address(virt);
        dir->unmap(virt);

        auto result = this->free_physical_frame(reinterpret_cast<void*>(physical));
        if (result.is_err()) return result;
    }

    space->m_used = false;
    return {};
}

ErrorOr<void*> MemoryManager::allocate_heap_region(size_t pages) {
    return this->allocate_from_region(m_heap_region, pages);
}

ErrorOr<void> MemoryManager::free_heap_region(void* start, size_t pages) {
    return this->free_from_region(m_heap_region, start, pages);
}

ErrorOr<void*> MemoryManager::allocate_kernel_region(size_t pages) {
    return this->allocate_from_region(m_kernel_region, pages);
}

ErrorOr<void> MemoryManager::free_kernel_region(void* start, size_t pages) {
    return this->free_from_region(m_kernel_region, start, pages);
}

void* MemoryManager::allocate(size_t size) {
    size_t pages = std::ceil_div(size, PAGE_SIZE);
    auto result = this->allocate_kernel_region(pages);
    if (result.is_err()) {
        return nullptr;
    }

    return result.value();
}

void MemoryManager::free(void* ptr) {
    auto dir = PageDirectory::kernel_page_directory();
    if (!dir->is_mapped(reinterpret_cast<u32>(ptr))) {
        return;
    }

    auto space = m_kernel_region.find_space(reinterpret_cast<u32>(ptr));
    if (!space) {
        return;
    }

    this->free_kernel_region(ptr, space->size() / PAGE_SIZE).value(); // discard the result
}

void* MemoryManager::map_physical_region(u32 start, size_t size) {
    u32 pages = std::ceil_div(size, PAGE_SIZE);
    auto dir = PageDirectory::kernel_page_directory();

    auto space = m_kernel_region.find_free_pages(pages);
    if (!space) {
        return nullptr;
    }

    for (u32 i = 0; i < pages; i++) {
        dir->map(space->address() + i * PAGE_SIZE, start + i * PAGE_SIZE, PageFlags::Writable);
    }

    space->m_used = true;
    space->m_perms = Permissions::Read | Permissions::Write;

    return reinterpret_cast<void*>(space->address());
}

void MemoryManager::unmap_physical_region(void* ptr) {
    auto dir = PageDirectory::kernel_page_directory();
    auto space = m_kernel_region.find_space(reinterpret_cast<u32>(ptr));
    if (!space) {
        return;
    }

    u32 pages = space->size() / PAGE_SIZE;
    for (u32 i = 0; i < pages; i++) {
        dir->unmap(space->address() + i * PAGE_SIZE);
    }

    space->m_used = false;
}

bool MemoryManager::is_mapped(void* addr) {
    auto dir = PageDirectory::kernel_page_directory();
    return dir->is_mapped(reinterpret_cast<u32>(addr));
}

u32 MemoryManager::get_physical_address(void* addr) {
    auto dir = PageDirectory::kernel_page_directory();
    return dir->get_physical_address(reinterpret_cast<u32>(addr));
}

}

// FIXME: Implement
int liballoc_lock() {
    return 0;
}

int liballoc_unlock() {
    return 0;
}

void* liballoc_alloc(size_t pages) {
    using namespace kernel::memory;

    size_t size = pages * PAGE_SIZE;
    if (s_kernel_heap_offset + size < INITIAL_KERNEL_HEAP_SIZE) {
        void* ptr = &s_kernel_heap[s_kernel_heap_offset];
        s_kernel_heap_offset += size;

        return ptr;
    }
    
    auto result = MemoryManager::instance()->allocate_heap_region(pages);

    if (result.is_err()) return nullptr;
    return result.value();
}

int liballoc_free(void* addr, size_t pages) {
    using namespace kernel::memory;

    // No need to do anything if the address is in the initial kernel heap
    if (addr >= &s_kernel_heap && addr < &s_kernel_heap[INITIAL_KERNEL_HEAP_SIZE]) {
        return 0;
    }

    auto mm = MemoryManager::instance();
    
    auto result = mm->free_heap_region(addr, pages);
    return result.is_err();
}

void* operator new(size_t size) {
    auto* ptr = kmalloc(size);
    std::memset(ptr, 0, size);

    return ptr;
}

void* operator new[](size_t size) {
    auto* p = kmalloc(size);
    std::memset(p, 0, size);

    return p;
}

void operator delete(void* p) { kfree(p); }
void operator delete[](void* p) { kfree(p); }

void operator delete(void* p, size_t) { kfree(p); }
void operator delete[](void* p, size_t) { kfree(p); }