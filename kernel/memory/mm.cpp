#include "kernel/common.h"
#include <kernel/memory/mm.h>
#include <kernel/memory/paging.h>
#include <kernel/memory/physical.h>
#include <kernel/memory/liballoc.h>

#include <kernel/panic.h>
#include <kernel/vga.h>
#include <kernel/serial.h>

static u8 s_initial_kernel_heap[INITIAL_KERNEL_HEAP_SIZE];
static u32 s_kernel_heap_offset = 0;

namespace kernel::memory {

void* allocate_from_initial_heap(size_t size) {
    if (s_kernel_heap_offset + size > INITIAL_KERNEL_HEAP_SIZE) {
        return nullptr;
    }

    void* ptr = &s_initial_kernel_heap[s_kernel_heap_offset];
    s_kernel_heap_offset += size;

    return ptr;
}

static MemoryManager s_memory_manager = MemoryManager();

MemoryManager::MemoryManager() {
    m_heap_region = Region(KERNEL_HEAP_ADDRESS, 0xFFFFFFFF - KERNEL_VIRTUAL_BASE);
    m_kernel_region = Region(KERNEL_VIRTUAL_BASE, KERNEL_HEAP_ADDRESS - KERNEL_VIRTUAL_BASE);
}

void MemoryManager::init(multiboot_info_t* header) {
    PhysicalMemoryManager::init(header->mem_lower, header->mem_upper);
    PageDirectory::init_kernel(s_memory_manager.m_kernel_region);

    cpu::set_idt_entry(14, reinterpret_cast<u32>(MemoryManager::page_fault_handler), 0x8E);
}

void MemoryManager::page_fault_handler(cpu::InterruptFrame* fr, u32 err) {
    u32 address = 0;
    asm volatile("mov %%cr2, %0" : "=r"(address));

    serial::printf("Page fault (address 0x%x) at EIP=0x%x:\n", address, fr->eip);
    PageFault fault = err;

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

    StackFrame* frame = kernel::get_stack_frame();
    serial::printf("Stack trace:\n");

    while (frame) {
        serial::printf(" - 0x%x\n", frame->eip);
        frame = frame->ebp;
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
    return TRY(pmm->allocate());
}

ErrorOr<void> MemoryManager::free_physical_frame(void* frame) {
    auto* pmm = PhysicalMemoryManager::instance();
    auto result = pmm->free(frame);

    if (result.is_err()) return result;
    return {};
}

ErrorOr<void*> MemoryManager::allocate_from_region(Region& region, size_t pages) {
    auto dir = PageDirectory::kernel_page_directory();

    auto space = region.allocate(pages, Permissions::Read | Permissions::Write);
    if (!space) return Error(ENOMEM);

    for (size_t i = 0; i < pages; i++) {
        void* frame = TRY(this->allocate_physical_frame());
        dir->map(
            space->address() + i * PAGE_SIZE, 
            reinterpret_cast<PhysicalAddress>(frame), 
            false,
            true
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

ErrorOr<void*> MemoryManager::map_physical_region(u32 start, size_t size) {
    u32 pages = size / PAGE_SIZE;
    if (pages * PAGE_SIZE < size) pages++;

    auto dir = PageDirectory::kernel_page_directory();

    auto space = m_kernel_region.find_free_pages(pages);
    if (!space) return Error(ENOMEM);

    for (u32 i = 0; i < pages; i++) {
        dir->map(space->address() + i * PAGE_SIZE, start + i * PAGE_SIZE, false, true);
    }

    space->m_used = true;
    return reinterpret_cast<void*>(space->address());
}

bool MemoryManager::is_mapped(void* addr) {
    auto dir = PageDirectory::kernel_page_directory();
    return dir->is_mapped(reinterpret_cast<u32>(addr));
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
    using namespace kernel;

    if (s_kernel_heap_offset + pages * PAGE_SIZE < INITIAL_KERNEL_HEAP_SIZE) {
        void* ptr = &s_initial_kernel_heap[s_kernel_heap_offset];
        s_kernel_heap_offset += pages * PAGE_SIZE;

        return ptr;
    }
    
    auto result = memory::MemoryManager::instance()->allocate_heap_region(pages);

    if (result.is_err()) return nullptr;
    return result.value();
}

int liballoc_free(void* addr, size_t pages) {
    // No need to do anything if the address is in the initial kernel heap
    if (addr >= &s_initial_kernel_heap && addr < &s_initial_kernel_heap[INITIAL_KERNEL_HEAP_SIZE]) {
        return 0;
    }

    auto mm = kernel::memory::MemoryManager::instance();
    
    auto result = mm->free_heap_region(addr, pages);
    return result.is_err();
}

void* operator new(size_t size) { return kmalloc(size); }
void* operator new[](size_t size) { return kmalloc(size); }

void operator delete(void* ptr) { kfree(ptr); }
void operator delete[](void* ptr) { kfree(ptr); }