#include <kernel/arch/x86_64/page_directory.h>
#include <kernel/memory/manager.h>

namespace kernel::arch {

static PageDirectory s_kernel_page_directory;
static PageDirectoryTableEntry s_kernel_page_directory_table_entry;

static PageDirectoryTable s_kernel_page_directory_table;
static ALIGNED(0x1000) PageDirectoryEntry s_kernel_page_directory_entries[512];

static PageTable s_kernel_page_tables[256];
static ALIGNED(0x1000) PageTableEntry s_kernel_page_table_entries[256][512];

PageTable* PageTable::create() {
    PageTable* table = new PageTable;
    ASSERT(table != nullptr, "Failed to allocate PageTable");

    table->m_entries = static_cast<PageTableEntry*>(MM->allocate_kernel_region(PAGE_SIZE));
    return table;
}

PageDirectoryTable* PageDirectoryTable::create() {
    PageDirectoryTable* table = new PageDirectoryTable;
    ASSERT(table != nullptr, "Failed to allocate PageDirectoryTable");

    table->m_entries = static_cast<PageDirectoryEntry*>(MM->allocate_kernel_region(PAGE_SIZE));
    return table;
}

PageTable* PageDirectoryTable::create_page_table(u32 pd, bool user) {
    PageDirectoryEntry& entry = m_entries[pd];
    if (entry.is_present()) {
        return m_tables[pd];
    }

    PageTable* table = PageTable::create();

    entry.set_present(true);
    entry.set_writable(true);
    entry.set_user(user);

    PhysicalAddress address = this->get_physical_address(table->address());
    entry.set_page_table_address(address);

    m_tables[pd] = table;
    return table;
}

PageTable* PageDirectoryTable::get_page_table(u32 pd) {
    return m_tables[pd];
}

VirtualAddress PageDirectoryTable::get_physical_address(VirtualAddress virt) const {
    u32 pd = get_page_directory_index(virt);
    u32 pt = get_page_table_index(virt);

    PageDirectoryEntry const& dir = m_entries[pd];
    if (!dir.is_present()) {
        return 0;
    }

    return m_tables[pd]->at(pt).get_physical_address();
}

PageTable* PageDirectory::create_page_table(u32 pdt, u32 pd, bool user) {
    PageDirectoryTableEntry& entry = m_entries[pdt];
    if (entry.is_present()) {
        PageDirectoryTable* table = m_tables[pdt];
        return table->create_page_table(pd, user);
    }

    PageDirectoryTable* table = PageDirectoryTable::create();

    entry.set_present(true);
    entry.set_writable(true);
    entry.set_user(user);

    PhysicalAddress address = this->get_physical_address(table->address());
    entry.set_page_directory_address(address);
    
    m_tables[pdt] = table;
    return table->create_page_table(pd, user);
}

PageTable* PageDirectory::get_page_table(u32 pdt, u32 pd) {
    PageDirectoryTable* table = m_tables[pdt];
    if (!table) {
        return nullptr;
    }

    return table->get_page_table(pd);
}

PageTableEntry const* PageDirectory::get_page_table_entry(VirtualAddress virt) const {
    u32 pdt = get_page_directory_table_index(virt);
    u32 pd = get_page_directory_index(virt);
    u32 pt = get_page_table_index(virt);

    PageDirectoryTable* table = m_tables[pdt];
    if (!table) {
        return nullptr;
    }

    PageTable* page_table = table->get_page_table(pd);
    if (!page_table) {
        return nullptr;
    }

    return &page_table->at(pt);
}

PageTableEntry* PageDirectory::get_page_table_entry(VirtualAddress virt) {
    u32 pdt = get_page_directory_table_index(virt);
    u32 pd = get_page_directory_index(virt);
    u32 pt = get_page_table_index(virt);

    PageDirectoryTable* table = m_tables[pdt];
    if (!table) {
        return nullptr;
    }

    PageTable* page_table = table->get_page_table(pd);
    if (!page_table) {
        return nullptr;
    }

    return &page_table->at(pt);
}

PhysicalAddress PageDirectory::get_physical_address(VirtualAddress virt) const {
    PageTableEntry const* entry = this->get_page_table_entry(virt);
    if (!entry) {
        return 0;
    }

    return entry->get_physical_address();
}

void PageDirectory::map(VirtualAddress virt, PhysicalAddress phys, PageFlags flags) {
    u32 pdt = get_page_directory_table_index(virt);
    u32 pd = get_page_directory_index(virt);
    u32 pt = get_page_table_index(virt);

    bool write = std::has_flag(flags, PageFlags::Write);
    bool user = std::has_flag(flags, PageFlags::User);

    PageTable* table = this->create_page_table(pdt, pd, false);
    PageTableEntry& entry = table->at(pt);

    entry.set_present(true);
    entry.set_writable(write);
    entry.set_user(user);
    entry.set_physical_address(phys);
}

void PageDirectory::unmap(VirtualAddress virt) {
    PageTableEntry* entry = this->get_page_table_entry(virt);
    if (!entry) {
        return;
    }

    entry->value = 0;
    // invlpg(reinterpret_cast<void*>(virt));
}

PhysicalAddress PageDirectory::cr3() const {
    return this->get_physical_address(reinterpret_cast<VirtualAddress>(m_entries));
}

void PageDirectory::switch_to() {}

PageDirectory* PageDirectory::kernel_page_directory() {
    return &s_kernel_page_directory;
}

void PageDirectory::create_kernel_page_directory(arch::BootInfo const& boot_info, memory::RegionAllocator& kernel_region_allocator) {
    auto* dir = PageDirectory::kernel_page_directory();

    u32 pd = get_page_directory_table_index(boot_info.kernel_virtual_base);
    // dir->m_entries = s_kernel_page_directory_table_entries;

    // dir->m_tables[pd] = &s_kernel_page_directory_table;
    // dir->m_entries[pd] = s_kernel_page_directory_table_entry;

    // s_kernel_page_directory_table.m_entries = s_kernel_page_directory_entries;

    // s_kernel_page_directory_entry.set_present(true);
    // s_kernel_page_directory_entry.set_writable(true);
    // s_kernel_page_directory_entry.set_user(false);

    // s_kernel_page_directory_entry.set_page_directory_address((size_t)&s_kernel_page_directory_entries - boot_info.kernel_virtual_base);

    for (size_t i = 0; i < 256; i++) {
        PageDirectoryEntry& entry = s_kernel_page_directory_entries[i];
        PageTable* table = s_kernel_page_tables + i;

        entry.set_present(true);
        entry.set_writable(true);
        entry.set_user(false);

        table->m_entries = s_kernel_page_table_entries[i];
        entry.set_page_table_address((size_t)s_kernel_page_table_entries[i] - boot_info.kernel_virtual_base);
    }

    size_t size = std::align_up(PAGE_SIZE, boot_info.kernel_size);
    for (size_t i = 0; i < size; i += PAGE_SIZE) {
        PhysicalAddress phys = boot_info.kernel_physical_base + i;
        VirtualAddress virt = boot_info.kernel_virtual_base + i;

        s_kernel_page_directory.map(virt, phys, PageFlags::Write);
    }
}

}