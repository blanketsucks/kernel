#include <kernel/memory/manager.h>
#include <kernel/memory/physical.h>
#include <kernel/memory/liballoc.h>
#include <kernel/process/scheduler.h>
#include <kernel/process/threads.h>
#include <kernel/process/process.h>
#include <kernel/posix/sys/mman.h>
#include <kernel/panic.h>
#include <kernel/vga.h>
#include <kernel/serial.h>

#include <std/cstring.h>
#include <std/utility.h>
#include <std/format.h>

namespace kernel::memory {

static u8 s_kernel_heap[INITIAL_KERNEL_HEAP_SIZE];
static u32 s_kernel_heap_offset = 0;

static MemoryManager s_memory_manager = MemoryManager();

MemoryManager::MemoryManager() {
    auto* dir = arch::PageDirectory::kernel_page_directory();

    m_heap_region_allocator = RegionAllocator({ KERNEL_HEAP_ADDRESS, 0xFFFFFFFF - KERNEL_HEAP_ADDRESS }, dir);
    m_kernel_region_allocator = RegionAllocator({ KERNEL_VIRTUAL_BASE, KERNEL_HEAP_ADDRESS - KERNEL_VIRTUAL_BASE }, dir);
}

void MemoryManager::init(arch::BootInfo const& boot_info) {
    PhysicalMemoryManager::init(boot_info);
    arch::PageDirectory::create_kernel_page_directory(boot_info, s_memory_manager.m_kernel_region_allocator);
}

void MemoryManager::page_fault_handler(arch::InterruptRegisters* regs) {
    u32 address = 0;
    asm volatile("mov %%cr2, %0" : "=r"(address));

    auto* thread = Scheduler::current_thread();
    if (!thread || thread->is_kernel()) {
        PageFault fault = regs->errno;

        // dbgln("\033[1;31mPage fault (address={:#p}) at EIP={:#p} ({}{}{}):\033[0m", address, regs->eip, fault.present ? 'P' : '-', fault.rw ? 'W' : 'R', fault.user ? 'U' : 'S');
        // dbgln("  \033[1;31mUnrecoverable page fault.\033[0m");

        kernel::panic("Kernel page fault");
    }

    // TODO: Print stacktrace
    auto* process = thread->process();
    process->handle_page_fault(regs, address);
}

MemoryManager* MemoryManager::instance() {
    return &s_memory_manager;
}

arch::PageDirectory* MemoryManager::kernel_page_directory() {
    return arch::PageDirectory::kernel_page_directory();
}

void* MemoryManager::allocate_physical_frame() {
    auto* pmm = PhysicalMemoryManager::instance();
    return pmm->allocate();
}

ErrorOr<void> MemoryManager::free_physical_frame(void* frame) {
    auto* pmm = PhysicalMemoryManager::instance();
    return pmm->free(frame);
}

void* MemoryManager::allocate(RegionAllocator& allocator, size_t size, PageFlags flags) {
    size = std::align_up(size, PAGE_SIZE);
    auto* region = allocator.allocate(size, PROT_READ | PROT_WRITE);
    if (!region) {
        return nullptr;
    }

    auto* page_directory = allocator.page_directory();
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        void* frame = this->allocate_physical_frame();
        if (!frame) {
            return nullptr;
        }

        page_directory->map(region->base() + i, reinterpret_cast<PhysicalAddress>(frame), flags);
    }

    return reinterpret_cast<void*>(region->base());
}

void* MemoryManager::allocate_at(RegionAllocator& allocator, uintptr_t address, size_t size, PageFlags flags) {
    size = std::align_up(size, PAGE_SIZE);
    auto region = allocator.allocate_at(address, size, PROT_READ | PROT_WRITE);
    if (!region) {
        return nullptr;
    }

    auto* page_directory = allocator.page_directory();
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        void* frame = this->allocate_physical_frame();
        if (!frame) {
            return nullptr;
        }

        page_directory->map(region->base() + i, reinterpret_cast<PhysicalAddress>(frame), flags);
    }

    return reinterpret_cast<void*>(region->base());
}

ErrorOr<void> MemoryManager::free(RegionAllocator& allocator, void* ptr, size_t size) {
    VirtualAddress address = reinterpret_cast<VirtualAddress>(ptr);
    auto* page_directory = allocator.page_directory();

    if (!page_directory->is_mapped(address)) {
        return Error(EINVAL);
    }

    auto* region = allocator.find_region(address);
    if (!region) {
        return Error(EINVAL);
    }

    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        PhysicalAddress physical = page_directory->get_physical_address(region->base() + i);
        page_directory->unmap(region->base() + i);

        TRY(this->free_physical_frame(reinterpret_cast<void*>(physical)));
    }

    allocator.free(region);
    return {};
}

void* MemoryManager::allocate_heap_region(size_t size) {
    return this->allocate(m_heap_region_allocator, size, PageFlags::Write);
}

ErrorOr<void> MemoryManager::free_heap_region(void* start, size_t size) {
    return this->free(m_heap_region_allocator, start, size);
}

void* MemoryManager::allocate_kernel_region(size_t size) {
    return this->allocate(m_kernel_region_allocator, size, PageFlags::Write);
}

ErrorOr<void> MemoryManager::free_kernel_region(void* start, size_t size) {
    return this->free(m_kernel_region_allocator, start, size);
}

void* MemoryManager::map_physical_region(uintptr_t start, size_t size) {
    size = std::align_up(size, PAGE_SIZE);
    auto* page_directory = arch::PageDirectory::kernel_page_directory();

    auto* region = m_kernel_region_allocator.allocate(size, PROT_READ | PROT_WRITE);
    if (!region) {
        return nullptr;
    }

    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        page_directory->map(region->base() + i, start + i, PageFlags::Write);
    }

    return reinterpret_cast<void*>(region->base());
}

void MemoryManager::unmap_physical_region(void* ptr) {
    auto* page_directory = arch::PageDirectory::kernel_page_directory();
    auto* region = m_kernel_region_allocator.find_region(reinterpret_cast<VirtualAddress>(ptr));

    if (!region) {
        return;
    }

    for (size_t i = 0; i < region->size(); i += PAGE_SIZE) {
        page_directory->unmap(region->base() + i);
    }

    m_kernel_region_allocator.free(region);
}

void* MemoryManager::map_from_page_directory(arch::PageDirectory* page_directory, void* ptr, size_t size) {
    size = std::align_up(size, PAGE_SIZE);
    auto* region = m_kernel_region_allocator.allocate(size, PROT_READ | PROT_WRITE);
    if (!region) {
        return nullptr;
    }

    auto* kernel_page_directory = arch::PageDirectory::kernel_page_directory();
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        PhysicalAddress address = page_directory->get_physical_address(reinterpret_cast<VirtualAddress>(ptr));
        kernel_page_directory->map(region->base() + i, address, PageFlags::Write);
    }

    return reinterpret_cast<void*>(region->base());
}



bool MemoryManager::is_mapped(void* addr) {
    auto dir = arch::PageDirectory::kernel_page_directory();
    return dir->is_mapped(reinterpret_cast<u32>(addr));
}

u32 MemoryManager::get_physical_address(void* addr) {
    auto dir = arch::PageDirectory::kernel_page_directory();
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
    
    return MM->allocate_heap_region(size);
}

int liballoc_free(void* addr, size_t pages) {
    using namespace kernel::memory;

    // No need to do anything if the address is in the initial kernel heap
    if (addr >= &s_kernel_heap && addr < &s_kernel_heap[INITIAL_KERNEL_HEAP_SIZE]) {
        return 0;
    }

    auto result = MM->free_heap_region(addr, pages * PAGE_SIZE);
    return result.is_err();
}

void* operator new(size_t size) {
    auto* ptr = kmalloc(size);
    memset(ptr, 0, size);

    return ptr;
}

void* operator new[](size_t size) {
    auto* p = kmalloc(size);
    memset(p, 0, size);

    return p;
}

void operator delete(void* p) { kfree(p); }
void operator delete[](void* p) { kfree(p); }

void operator delete(void* p, size_t) { kfree(p); }
void operator delete[](void* p, size_t) { kfree(p); }