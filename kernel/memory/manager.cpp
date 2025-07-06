#include <kernel/boot/boot_info.h>
#include <kernel/memory/manager.h>
#include <kernel/memory/pmm.h>
#include <kernel/memory/liballoc.h>
#include <kernel/process/scheduler.h>
#include <kernel/process/threads.h>
#include <kernel/process/process.h>
#include <kernel/posix/sys/mman.h>

#include <std/cstring.h>
#include <std/utility.h>
#include <std/format.h>

namespace kernel::memory {

static u8 s_kernel_heap[INITIAL_KERNEL_HEAP_SIZE];
static u32 s_kernel_heap_offset = 0;

static MemoryManager* s_mm = nullptr;

// We need a dummy page to return in case the physical pages are not initialized yet
// and that is when we are creating the kernel page directory. Realistically, we should have
// a better way to handle this.
static PhysicalPage s_dummy_page = {};

size_t MemoryManager::current_kernel_heap_offset() {
    return s_kernel_heap_offset;
}

MemoryManager::MemoryManager() {
    auto* dir = arch::PageDirectory::kernel_page_directory();

    VirtualAddress heap_base = g_boot_info->kernel_heap_base;
    VirtualAddress virtual_base = g_boot_info->kernel_virtual_base;

    m_heap_region_allocator = RegionAllocator::create({ heap_base, MAX_VIRTUAL_ADDRESS - heap_base }, dir);
    m_kernel_region_allocator = RegionAllocator::create({ virtual_base + g_boot_info->kernel_size, heap_base - virtual_base - g_boot_info->kernel_size }, dir);
}

void MemoryManager::init() {
    s_mm = new MemoryManager();
    s_mm->initialize();
}

void MemoryManager::initialize() {
    m_pmm = PhysicalMemoryManager::create(*g_boot_info);
    arch::PageDirectory::create_kernel_page_directory(*g_boot_info, *m_kernel_region_allocator);

    this->create_physical_pages();
}

void MemoryManager::create_physical_pages() {
    PhysicalAddress highest_address = 0;
    for (const auto& region : m_pmm->regions()) {
        if (region.end() > highest_address) {
            highest_address = region.end();
        }
    }

    m_physical_pages_count = highest_address / PAGE_SIZE;

    size_t allocation_size = m_physical_pages_count * sizeof(PhysicalPage);
    void* region = this->allocate_kernel_region(allocation_size);

    memset(region, 0, allocation_size);
    m_physical_pages = reinterpret_cast<PhysicalPage*>(region);
}

PhysicalPage* MemoryManager::get_physical_page(PhysicalAddress address) {
    if (!m_physical_pages) {
        return &s_dummy_page;
    }

    if (address % PAGE_SIZE != 0) {
        return nullptr;
    }

    size_t index = address / PAGE_SIZE;
    if (index >= m_physical_pages_count) {
        return nullptr;
    }

    return &m_physical_pages[index];
}

void MemoryManager::page_fault_handler(arch::InterruptRegisters* regs) {
    VirtualAddress address = 0;
    asm volatile("mov %%cr2, %0" : "=r"(address));

    // Assume that the address is a null pointer dereference if it's less than a page size.
    // No user process can ever allocate in the first page of memory so this is a safe assumption
    bool is_null_pointer_dereference = address < PAGE_SIZE;

    auto* thread = Thread::current();
    if (!thread || thread->is_kernel()) {
        PageFault fault = regs->errno;

        if (is_null_pointer_dereference) {
            dbgln("\033[1;31mNull pointer dereference (address={:#p}) at IP={:#p} ({}{}{}):\033[0m", address, regs->ip(), fault.present ? 'P' : '-', fault.rw ? 'W' : 'R', fault.user ? 'U' : 'S');
        } else {
            dbgln("\033[1;31mPage fault (address={:#p}) at IP={:#p} ({}{}{}):\033[0m", address, regs->ip(), fault.present ? 'P' : '-', fault.rw ? 'W' : 'R', fault.user ? 'U' : 'S');
        }

        dbgln("  \033[1;31mUnrecoverable page fault.\033[0m");

        kernel::print_stack_trace();
        
        asm volatile("cli");
        asm volatile("hlt");
    }

    // TODO: Print stacktrace
    auto* process = thread->process();
    process->handle_page_fault(regs, address);
}

MemoryManager* MemoryManager::instance() {
    return s_mm;
}

arch::PageDirectory* MemoryManager::kernel_page_directory() {
    return arch::PageDirectory::kernel_page_directory();
}

void* MemoryManager::allocate_page_frame() {
    auto result = m_pmm->allocate();
    if (result.is_err()) {
        return nullptr;
    }

    return result.value();
}

void* MemoryManager::allocate_contiguous_frames(size_t count) {
    auto result = m_pmm->allocate_contiguous(count);
    if (result.is_err()) {
        return nullptr;
    }

    return result.value();
}

ErrorOr<void> MemoryManager::free_page_frame(void* frame) {
    return m_pmm->free(frame, 1);
}

ErrorOr<void> MemoryManager::map_region(arch::PageDirectory* page_directory, Region* region, PageFlags flags) {
    ScopedSpinLock lock(m_lock);
    if (this->try_allocate_contiguous(page_directory, region, flags)) {
        return {};
    }

    for (size_t i = 0; i < region->size(); i += PAGE_SIZE) {
        void* frame = this->allocate_page_frame();
        if (!frame) {
            return Error(ENOMEM);
        }

        PhysicalPage* page = this->get_physical_page(reinterpret_cast<PhysicalAddress>(frame));
        page->ref_count++;
        
        page_directory->map(region->base() + i, reinterpret_cast<PhysicalAddress>(frame), flags);
    }

    return {};
}

void* MemoryManager::allocate(RegionAllocator& allocator, size_t size, PageFlags flags) {
    // FIXME: Acquiring the lock *sometimes* hangs when trying to allocate the DMA region for the control USB pipe.
    ScopedSpinLock lock(m_lock);

    size = std::align_up(size, PAGE_SIZE);
    auto* region = allocator.allocate(size, PROT_READ | PROT_WRITE);
    if (!region) {
        return nullptr;
    }

    auto* page_directory = allocator.page_directory();
    if (this->try_allocate_contiguous(page_directory, region, flags)) {
        return reinterpret_cast<void*>(region->base());
    }

    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        void* frame = this->allocate_page_frame();
        if (!frame) {
            return nullptr;
        }

        PhysicalAddress physical_address = reinterpret_cast<PhysicalAddress>(frame);
        PhysicalPage* page = this->get_physical_page(physical_address);

        page->ref_count++;
        page_directory->map(region->base() + i, physical_address, flags);
    }

    return reinterpret_cast<void*>(region->base());
}

bool MemoryManager::try_allocate_contiguous(arch::PageDirectory* page_directory, Region* region, PageFlags flags) {
    void* frame = this->allocate_contiguous_frames(region->size() / PAGE_SIZE);
    if (!frame) {
        return false;
    }
    
    PhysicalAddress physical_address = reinterpret_cast<PhysicalAddress>(frame);
    for (size_t i = 0; i < region->size(); i += PAGE_SIZE) {
        PhysicalPage* page = this->get_physical_page(physical_address + i);
        page->ref_count++;

        page_directory->map(region->base() + i, physical_address + i, flags);
    }

    return true;
}

void* MemoryManager::allocate_at(RegionAllocator& allocator, VirtualAddress address, size_t size, PageFlags flags) {
    size = std::align_up(size, PAGE_SIZE);
    auto region = allocator.allocate_at(address, size, PROT_READ | PROT_WRITE);
    if (!region) {
        return nullptr;
    }

    auto* page_directory = allocator.page_directory();
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        void* frame = this->allocate_page_frame();
        if (!frame) {
            return nullptr;
        }

        PhysicalPage* page = this->get_physical_page(reinterpret_cast<PhysicalAddress>(frame));
        page->ref_count++;

        page_directory->map(region->base() + i, reinterpret_cast<PhysicalAddress>(frame), flags);
    }

    return reinterpret_cast<void*>(region->base());
}

ErrorOr<void> MemoryManager::free(RegionAllocator& allocator, Region* region) {
    auto* page_directory = allocator.page_directory();
    if (!page_directory->is_mapped(region->base())) {
        return Error(EINVAL);
    }

    for (size_t i = 0; i < region->size(); i += PAGE_SIZE) {
        PhysicalAddress physical = page_directory->get_physical_address(region->base() + i);
        page_directory->unmap(region->base() + i);

        PhysicalPage* page = this->get_physical_page(physical);
        page->ref_count--;

        if (page->ref_count == 0) {
            TRY(this->free_page_frame(reinterpret_cast<void*>(physical)));
        }
    }

    allocator.free(region);
    return {};
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

        PhysicalPage* page = this->get_physical_page(physical);
        page->ref_count--;

        if (page->ref_count == 0) {
            TRY(this->free_page_frame(reinterpret_cast<void*>(physical)));
        }
    }

    allocator.free(region);
    return {};
}

void* MemoryManager::allocate_heap_region(size_t size) {
    return this->allocate(*m_heap_region_allocator, size, PageFlags::Write);
}

ErrorOr<void> MemoryManager::free_heap_region(void* start, size_t size) {
    return this->free(*m_heap_region_allocator, start, size);
}

void* MemoryManager::allocate_kernel_region(size_t size) {
    return this->allocate(*m_kernel_region_allocator, size, PageFlags::Write);
}

ErrorOr<void> MemoryManager::free_kernel_region(void* ptr, size_t size) {
    return this->free(*m_kernel_region_allocator, ptr, size);
}

void* MemoryManager::allocate_dma_region(size_t size) {
    return this->allocate(*m_kernel_region_allocator, size, PageFlags::Write | PageFlags::CacheDisable);
}

ErrorOr<void> MemoryManager::free_dma_region(void* ptr, size_t size) {
    return this->free(*m_kernel_region_allocator, ptr, size);
}

void* MemoryManager::map_physical_region(void* ptr, size_t size) {
    size = std::align_up(size, PAGE_SIZE);
    auto* page_directory = arch::PageDirectory::kernel_page_directory();

    auto* region = m_kernel_region_allocator->allocate(size, PROT_READ | PROT_WRITE);
    if (!region) {
        return nullptr;
    }

    PhysicalAddress start = reinterpret_cast<PhysicalAddress>(ptr);
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        page_directory->map(region->base() + i, start + i, PageFlags::Write);
    }

    return reinterpret_cast<void*>(region->base());
}

void MemoryManager::unmap_physical_region(void* ptr) {
    auto* page_directory = arch::PageDirectory::kernel_page_directory();
    auto* region = m_kernel_region_allocator->find_region(reinterpret_cast<VirtualAddress>(ptr));

    if (!region) {
        return;
    }

    for (size_t i = 0; i < region->size(); i += PAGE_SIZE) {
        page_directory->unmap(region->base() + i);
    }

    m_kernel_region_allocator->free(region);
}

void* MemoryManager::map_from_page_directory(arch::PageDirectory* page_directory, void* ptr, size_t size) {
    size = std::align_up(size, PAGE_SIZE);
    auto* region = m_kernel_region_allocator->allocate(size, PROT_READ | PROT_WRITE);
    if (!region) {
        return nullptr;
    }

    auto* kernel_page_directory = arch::PageDirectory::kernel_page_directory();
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        PhysicalAddress address = page_directory->get_physical_address(reinterpret_cast<VirtualAddress>(ptr) + i);
        kernel_page_directory->map(region->base() + i, address, PageFlags::Write);
    }

    return reinterpret_cast<void*>(region->base());
}

void MemoryManager::copy_physical_memory(void* d, void* s, size_t size) {
    void* dst = this->map_physical_region(d, size);
    void* src = this->map_physical_region(s, size);

    memcpy(dst, src, size);

    this->unmap_physical_region(dst);
    this->unmap_physical_region(src);
}

bool MemoryManager::is_mapped(void* addr) {
    auto dir = arch::PageDirectory::kernel_page_directory();
    return dir->is_mapped(reinterpret_cast<VirtualAddress>(addr));
}

PhysicalAddress MemoryManager::get_physical_address(void* addr) {
    auto dir = arch::PageDirectory::kernel_page_directory();
    return dir->get_physical_address(reinterpret_cast<VirtualAddress>(addr));
}

TemporaryMapping::TemporaryMapping(arch::PageDirectory& page_directory, void* ptr, size_t size) : m_size(size) {
    m_ptr = (u8*)s_mm->map_from_page_directory(&page_directory, ptr, size);
}

TemporaryMapping::~TemporaryMapping() {
    s_mm->unmap_physical_region(m_ptr);
}

}

// FIXME: Implement
int liballoc_lock() {
    using namespace kernel::memory;
    if (s_mm) {
        s_mm->liballoc_lock().lock();
    }

    return 0;
}

int liballoc_unlock() {
    using namespace kernel::memory;
    if (s_mm) {
        s_mm->liballoc_lock().unlock();
    }

    return 0;
}

void* liballoc_alloc(size_t pages) {
    using namespace kernel::memory;

    size_t size = pages * kernel::PAGE_SIZE;
    if (s_kernel_heap_offset + size < kernel::INITIAL_KERNEL_HEAP_SIZE) {
        void* ptr = &s_kernel_heap[s_kernel_heap_offset];
        s_kernel_heap_offset += size;

        return ptr;
    }
    
    return MM->allocate_heap_region(size);
}

int liballoc_free(void* addr, size_t pages) {
    using namespace kernel::memory;

    // No need to do anything if the address is in the initial kernel heap
    if (addr >= &s_kernel_heap && addr < &s_kernel_heap[kernel::INITIAL_KERNEL_HEAP_SIZE]) {
        return 0;
    }

    auto result = MM->free_heap_region(addr, pages * kernel::PAGE_SIZE);
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