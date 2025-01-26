#include "kernel/common.h"
#include <kernel/arch/x86/page_directory.h>
#include <kernel/memory/region.h>
#include <kernel/posix/sys/mman.h>
#include <kernel/serial.h>
#include <kernel/panic.h>

#include <std/string.h>

namespace kernel::arch {

static PageDirectory s_kernel_page_directory;
static ALIGNED(0x1000) PageDirectoryEntry s_kernel_page_directory_entries[1024];

static PageTable s_kernel_page_tables[256];
static ALIGNED(0x1000) PageTableEntry s_kernel_page_table_entries[256][1024];

static PageDirectory* s_current_page_directory = nullptr;

PageTable* PageTable::create() {
    PageTable* table = new PageTable;
    ASSERT(table != nullptr, "Failed to allocate PageTable");

    table->m_entries = static_cast<PageTableEntry*>(MM->allocate_kernel_region(PAGE_SIZE));
    return table;
}

void invlpg(void* addr) {
    asm volatile("invlpg (%0)" :: "r"(addr) : "memory");
}

PageDirectory::PageDirectory() : m_type(Kernel) {
    m_entries = s_kernel_page_directory_entries;
}

PageDirectory* PageDirectory::create_user_page_directory() {
    PageDirectory* dir = new PageDirectory(User);

    auto* entries = static_cast<PageDirectoryEntry*>(MM->allocate_kernel_region(1));
    dir->m_entries = entries;

    memset(entries, 0, sizeof(PageDirectoryEntry) * 1024);
    
    // Share the top 1GB of kernel page tables (0xC0000000 - 0xFFFFFFFF)
    for(auto i = 768; i < 1024; i++) {
        entries[i].value = s_kernel_page_directory_entries[i].value;
    }

    return dir;
}

void PageDirectory::clone_into(PageDirectory* other) const {
    for (u32 i = 0; i < 768; i++) {
        auto entry = m_entries[i];
        if (!entry.is_present()) {
            continue;
        }

        auto* page_table = this->get_page_table(i);
        auto* cloned = PageTable::create();

        for (u32 j = 0; j < 1024; j++) {
            auto& entry = page_table->at(j);
            if (!entry.is_present()) {
                continue;
            }

            auto& cloned_entry = cloned->at(j);
            cloned_entry.value = entry.value;
            
            cloned_entry.set_physical_address((PhysicalAddress)MM->allocate_physical_frame());
        }

        // It's safe to modify `entry` since we don't take it by reference
        entry.set_page_table_address(this->get_physical_address(cloned->address()));

        other->m_entries[i] = entry;
        other->m_tables[i] = cloned;
    }
}

PageDirectory* PageDirectory::clone() const {
    if (this->is_kernel()) {
        return nullptr;
    }

    auto* page_directory = this->create_user_page_directory();
    this->clone_into(page_directory);
    
    return page_directory;
}

PageTable* PageDirectory::get_page_table(u32 pd) {
    if (pd >= 0x300 || this->is_kernel()) {
        return &s_kernel_page_tables[pd - 0x300];
    }

    return m_tables[pd];
}

PageTable const* PageDirectory::get_page_table(u32 pd) const {
    if (pd >= 0x300 || this->is_kernel()) {
        return &s_kernel_page_tables[pd - 0x300];
    }

    return m_tables[pd];
}

PageTable* PageDirectory::create_page_table(u32 pd, bool user) {
    PageDirectoryEntry& dir = m_entries[pd];
    if (dir.is_present()) {
        return this->get_page_table(pd);
    }

    PageTable* table = PageTable::create();

    dir.set_present(true);
    dir.set_writable(true);
    dir.set_user(user);

    u32 address = this->get_physical_address(table->address());
    dir.set_page_table_address(address);

    m_tables[pd] = table;
    return table;
}

void PageDirectory::map(VirtualAddress virt, PhysicalAddress phys, PageFlags flags) {
    u32 pd = get_page_directory_index(virt);
    u32 pt = get_page_table_index(virt);

    bool write = std::has_flag(flags, PageFlags::Write);
    bool user = std::has_flag(flags, PageFlags::User);

    PageTable* table = this->create_page_table(pd, user);
    PageTableEntry& entry = table->at(pt);

    entry.set_present(true);
    entry.set_writable(write);
    entry.set_user(user);
    entry.set_physical_address(phys);
}

void PageDirectory::unmap(VirtualAddress virt) {
    u32 pd = get_page_directory_index(virt);
    u32 pt = get_page_table_index(virt);

    PageDirectoryEntry& dir = m_entries[pd];
    if (!dir.is_present()) {
        return;
    }

    PageTableEntry& entry = this->get_page_table(pd)->at(pt);
    entry.value = 0;

    invlpg(reinterpret_cast<void*>(virt));
}

PageTableEntry const* PageDirectory::get_page_table_entry(VirtualAddress virt) const {
    u32 pd = get_page_directory_index(virt);
    u32 pt = get_page_table_index(virt);

    PageDirectoryEntry const& dir = m_entries[pd];
    if (!dir.is_present()) {
        return nullptr;
    }

    return &this->get_page_table(pd)->at(pt);
}

PhysicalAddress PageDirectory::get_physical_address(VirtualAddress virt) const {
    PageTableEntry const* entry = this->get_page_table_entry(virt);
    if (!entry) {
        return 0;
    }

    return entry->get_physical_address();
}

bool PageDirectory::is_mapped(VirtualAddress virt) const {
    u32 pd = get_page_directory_index(virt);
    u32 pt = get_page_table_index(virt);

    PageDirectoryEntry const& dir = m_entries[pd];
    if (!dir.is_present()) {
        return false;
    }

    PageTableEntry const& entry = this->get_page_table(pd)->at(pt);
    return entry.is_present();
}

void PageDirectory::clear() {
    memset(m_entries, 0, sizeof(PageDirectoryEntry) * 1024);
}

VirtualAddress PageDirectory::cr3() const {
    return this->get_physical_address(reinterpret_cast<VirtualAddress>(m_entries));
}

void PageDirectory::switch_to() {
    s_current_page_directory = this;
    asm volatile("mov %%eax, %%cr3" :: "a"(this->cr3()));
}

PageDirectory* PageDirectory::kernel_page_directory() {
    return &s_kernel_page_directory;
}

void PageDirectory::create_kernel_page_directory(arch::BootInfo const& boot_info, memory::RegionAllocator& kernel_region_allocator) {
    PageDirectory* dir = PageDirectory::kernel_page_directory();
    dir->clear();
    
    for (u32 i = 0; i < 256; i++) {
        PageTable* table = &s_kernel_page_tables[i];
        table->m_entries = s_kernel_page_table_entries[i];

        memset(table->m_entries, 0, sizeof(PageTableEntry) * 1024);
        dir->m_tables[i] = table;
    }

    // Map the upper 1GB of the kernel page tables (0xC0000000 - 0xFFFFFFFF)
    for (u32 i = 0; i < 256; i++) {
        dir->m_entries[i + 0x300].value = ((size_t)&s_kernel_page_table_entries[i] - KERNEL_VIRTUAL_BASE) | 0x3;
    }

    size_t size = std::align_up(boot_info.kernel_size, PAGE_SIZE);
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        PhysicalAddress phys = boot_info.kernel_physical_base + i;
        VirtualAddress virt = boot_info.kernel_virtual_base + i;

        dir->map(virt, phys, PageFlags::Write); 
    }

    // Reserve space in the MemoryManager's kernel region so we don't end up getting some forbidden address when trying to allocate from it.
    kernel_region_allocator.reserve(static_cast<u32>(boot_info.kernel_virtual_base), size, PROT_READ | PROT_WRITE);

    dir->map(VIRTUAL_VGA_ADDRESS, PHYSICAL_VGA_ADDRESS, PageFlags::Write);
    dir->switch_to();
}

}