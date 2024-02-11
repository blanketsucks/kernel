#include <kernel/memory/paging.h>
#include <kernel/memory/physical.h>
#include <kernel/memory/mm.h>
#include <kernel/vga.h>

#include <std/string.h>
#include <kernel/panic.h>
#include <kernel/serial.h>

extern "C" u64 _kernel_start;
extern "C" u64 _kernel_end;

namespace kernel::memory {

static PageDirectory s_kernel_page_directory;
static ALIGNED(0x1000) PageDirectoryEntry s_kernel_page_directory_entries[1024];

static PageTable s_kernel_page_tables[256];
static ALIGNED(0x1000) PageTableEntry s_kernel_page_table_entries[256][1024];

static PageDirectory* s_current_page_directory = nullptr;

PageTable* PageTable::create() {
    PageTable* table = new PageTable;
    ASSERT(table != nullptr, "Failed to allocate PageTable");

    table->m_entries = static_cast<PageTableEntry*>(MM->allocate_kernel_region(1).value());
    return table;
}

void invlpg(void* addr) {
    asm volatile("invlpg (%0)" :: "r"(addr) : "memory");
}

PageDirectory::PageDirectory() : m_type(Kernel) {
    m_entries = s_kernel_page_directory_entries;
}

PageDirectory* PageDirectory::create_user() {
    PageDirectory* dir = new PageDirectory(User);

    auto* entries = static_cast<PageDirectoryEntry*>(MM->allocate_kernel_region(1).value());
    dir->m_entries = entries;

    std::memset(entries, 0, sizeof(PageDirectoryEntry) * 1024);
    
    // Share the top 1GB of kernel page tables (0xC0000000 - 0xFFFFFFFF)
    for(auto i = 768; i < 1024; i++) {
        entries[i].value = s_kernel_page_directory_entries[i - 0x300].value;
    }

    return dir;
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

    u32 addr = this->get_physical_address(table->address());
    dir.set_page_table_address(addr);

    m_tables[pd] = table;
    return table;
}

void PageDirectory::map(VirtualAddress virt, PhysicalAddress phys, bool user, bool writable) {
    u32 pd = get_page_directory_index(virt);
    u32 pt = get_page_table_index(virt);

    PageTable* table = this->create_page_table(pd, user);
    PageTableEntry& entry = table->at(pt);

    entry.set_present(true);
    entry.set_writable(writable);
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

PhysicalAddress PageDirectory::get_physical_address(VirtualAddress virt) const {
    u32 pd = get_page_directory_index(virt);
    u32 pt = get_page_table_index(virt);

    PageDirectoryEntry const& dir = m_entries[pd];
    if (!dir.is_present()) {
        return 0;
    }

    PageTableEntry const& entry = this->get_page_table(pd)->at(pt);
    return entry.get_physical_address();
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
    for (u32 i = 0; i < 1024; i++) {
        m_entries[i].value = 0;
    }
}

u32 PageDirectory::cr3() const {
    return this->get_physical_address(reinterpret_cast<VirtualAddress>(m_entries));
}

void PageDirectory::switch_to() {
    s_current_page_directory = this;
    asm volatile("mov %%eax, %%cr3" :: "a"(this->cr3()));
}

PageDirectory* PageDirectory::current() {
    return s_current_page_directory;
}

PageDirectory* PageDirectory::kernel_page_directory() {
    return &s_kernel_page_directory;
}

void PageDirectory::init_kernel(Region& kernel_region) {
    PageDirectory* dir = PageDirectory::kernel_page_directory();
    dir->clear();

    for (u32 i = 0; i < 256; i++) {
        PageTable* table = &s_kernel_page_tables[i];
        table->m_entries = s_kernel_page_table_entries[i];

        std::memset(table->m_entries, 0, sizeof(PageTableEntry) * 1024);
        dir->m_tables[i] = table;
    }

    // Map the upper 1GB of the kernel page tables (0xC0000000 - 0xFFFFFFFF)
    for (u32 i = 0; i < 256; i++) {
        dir->m_entries[i + 0x300].value = ((size_t)&s_kernel_page_table_entries[i] - KERNEL_VIRTUAL_BASE) | 0x3;
    }

    u64 kernel_start = reinterpret_cast<u64>(&_kernel_start);
    u64 kernel_end = reinterpret_cast<u64>(&_kernel_end);

    // FIXME: This maps all kernel sections to writable which is not true for some like .rodata, .text, etc.
    for (u64 i = kernel_start; i < kernel_end; i += 0x1000) {
        dir->map(i, i - KERNEL_VIRTUAL_BASE, false, true);
    }

    // Reserve space in the MemoryManager's kernel region so we don't end up getting some forbidden address
    // when trying to allocate from it.
    kernel_region.reserve(
        static_cast<u32>(kernel_start), 
        static_cast<u32>(kernel_end - kernel_start), 
        Permissions::Read | Permissions::Write
    );

    dir->map(VIRTUAL_VGA_ADDRESS, PHYSICAL_VGA_ADDRESS, false, true);
    dir->switch_to();
}

}