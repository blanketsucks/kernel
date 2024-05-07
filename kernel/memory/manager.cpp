#include <kernel/memory/manager.h>
#include <kernel/memory/physical.h>
#include <kernel/memory/liballoc.h>
#include <kernel/process/scheduler.h>
#include <kernel/process/threads.h>
#include <kernel/process/process.h>
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
    auto* dir = arch::PageDirectory::kernel_page_directory();

    m_heap_region = Region(KERNEL_HEAP_ADDRESS, 0xFFFFFFFF, dir);
    m_kernel_region = Region(KERNEL_VIRTUAL_BASE, KERNEL_HEAP_ADDRESS, dir);
}

void MemoryManager::init(arch::BootInfo const& boot_info) {
    PhysicalMemoryManager::init(boot_info);
    arch::PageDirectory::create_kernel_page_directory(boot_info, s_memory_manager.m_kernel_region);
}

void MemoryManager::page_fault_handler(arch::InterruptRegisters* regs) {
    u32 address = 0;
    asm volatile("mov %%cr2, %0" : "=r"(address));

    PageFault fault = regs->errno;
    serial::printf("Page fault (address=0x%x) at EIP=%#x (%c%c%c):\n", address, regs->eip, fault.present ? 'P' : '-', fault.rw ? 'W' : 'R', fault.user ? 'U' : 'S');

    auto* thread = Scheduler::current_thread();
    if (!thread || thread->is_kernel()) {
        kernel::panic("Kernel page fault");
    }

    // TODO: Print stacktrace
    auto* process = thread->process();
    serial::printf("  In process: '%s' (PID %u)\n", process->name().data(), process->id());

    // FIXME: Send proper signal once that is implemented
    process->kill();
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

void* MemoryManager::allocate(Region& region, size_t size, PageFlags flags) {
    size = std::align_up(size, PAGE_SIZE);
    auto space = region.allocate(size, Permissions::Read | Permissions::Write, true);
    
    if (!space) {
        return nullptr;
    }

    auto dir = region.page_directory();
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        void* frame = this->allocate_physical_frame();
        if (!frame) return nullptr;

        dir->map(space->address() + i, reinterpret_cast<PhysicalAddress>(frame), flags);
    }

    return reinterpret_cast<void*>(space->address());
}

void* MemoryManager::allocate_at(Region& region, uintptr_t address, size_t size, PageFlags flags) {
    size = std::align_up(size, PAGE_SIZE);
    auto space = region.allocate_at(address, size, Permissions::Read | Permissions::Write);

    if (!space) {
        return nullptr;
    }

    auto dir = region.page_directory();
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        void* frame = this->allocate_physical_frame();
        if (!frame) return nullptr;

        dir->map(space->address() + i, reinterpret_cast<PhysicalAddress>(frame), flags);
    }

    return reinterpret_cast<void*>(space->address());
}

ErrorOr<void> MemoryManager::free(Region& region, void* ptr, size_t size) {
    if (!this->is_mapped(ptr)) {
        return Error(EINVAL);
    }

    auto space = region.find_space(reinterpret_cast<u32>(ptr));
    if (!space) {
        return Error(EINVAL);
    }

    auto dir = region.page_directory();
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        u32 virt = space->address() + i;

        PhysicalAddress physical = dir->get_physical_address(virt);
        dir->unmap(virt);

        TRY(this->free_physical_frame(reinterpret_cast<void*>(physical)));
    }

    space->m_used = false;
    return {};
}

void* MemoryManager::allocate_heap_region(size_t size) {
    return this->allocate(m_heap_region, size, PageFlags::Write);
}

ErrorOr<void> MemoryManager::free_heap_region(void* start, size_t size) {
    return this->free(m_heap_region, start, size);
}

void* MemoryManager::allocate_kernel_region(size_t size) {
    return this->allocate(m_kernel_region, size, PageFlags::Write);
}

ErrorOr<void> MemoryManager::free_kernel_region(void* start, size_t size) {
    return this->free(m_kernel_region, start, size);
}

void* MemoryManager::map_physical_region(uintptr_t start, size_t size) {
    size_t pages = std::ceil_div(size, static_cast<size_t>(PAGE_SIZE));
    auto dir = arch::PageDirectory::kernel_page_directory();

    auto space = m_kernel_region.find_free_pages(pages);
    if (!space) {
        return nullptr;
    }

    for (size_t i = 0; i < pages; i++) {
        dir->map(space->address() + i * PAGE_SIZE, start + i * PAGE_SIZE, PageFlags::Write);
    }

    space->m_used = true;
    space->m_perms = Permissions::Read | Permissions::Write;

    return reinterpret_cast<void*>(space->address());
}

void MemoryManager::unmap_physical_region(void* ptr) {
    auto dir = arch::PageDirectory::kernel_page_directory();
    auto space = m_kernel_region.find_space(reinterpret_cast<u32>(ptr));
    if (!space) {
        return;
    }

    size_t pages = space->size() / PAGE_SIZE;
    for (size_t i = 0; i < pages; i++) {
        dir->unmap(space->address() + i * PAGE_SIZE);
    }

    space->m_used = false;
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