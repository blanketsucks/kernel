#include <kernel/arch/x86_64/page_directory.h>
#include <kernel/arch/cpu.h>
#include <kernel/memory/manager.h>

#include <std/format.h>

namespace kernel::arch {

static constexpr size_t HHDM_MAPPING_SIZE = 1 * GB;
static constexpr size_t HHDM_PML4_ENTRY_COUNT = HHDM_MAPPING_SIZE / GB;

static PageDirectory s_kernel_page_directory;

PageDirectory* PageDirectory::create_user_page_directory() {
    PageDirectory* dir = new PageDirectory();
    dir->set_type(User);

    dir->m_pml4 = {
        reinterpret_cast<PML4Entry*>((u8*)MM->allocate_page_frame() + g_boot_info->hhdm)
    };

    size_t hhdm_pml4e = PML4::index(g_boot_info->hhdm);
    for (size_t i = 0; i < HHDM_PML4_ENTRY_COUNT; i++) {
        dir->m_pml4.entries[hhdm_pml4e + i] = s_kernel_page_directory.m_pml4.entries[hhdm_pml4e + i];
    }

    size_t kernel_pml4e = PML4::index(g_boot_info->kernel_virtual_base);
    dir->m_pml4.entries[kernel_pml4e] = s_kernel_page_directory.m_pml4.entries[kernel_pml4e];

    return dir;
}

PageTableEntry* PageDirectory::walk_page_table(VirtualAddress virt, bool create, bool user) {
    return walk_page_table<PML4>(m_pml4, virt, create, user);
}

template<typename T> PageTableEntry* PageDirectory::walk_page_table(T table, VirtualAddress virt, bool create, bool user) {
    using Entries = decltype(T::Next::entries);

    auto index = T::index(virt);
    auto& entry = table.entries[index];

    if (!entry.is_present()) {
        if (!create) {
            return nullptr;
        }

        auto* frame = MM->allocate_page_frame();
        if (!frame) {
            return nullptr;
        }

        entry.set_present(true);
        entry.set_writable(true);
        entry.set_user(user);

        entry.set_physical_address(reinterpret_cast<PhysicalAddress>(frame));
        memset(reinterpret_cast<void*>(entry.physical_address() + g_boot_info->hhdm), 0, PAGE_SIZE);
    }

    VirtualAddress address = entry.physical_address() + g_boot_info->hhdm;
    auto* entries = reinterpret_cast<Entries>(address);

    return walk_page_table<typename T::Next>({ entries }, virt, create, user);
}

template<> PageTableEntry* PageDirectory::walk_page_table(
    PageDirectoryTable table, VirtualAddress virt, bool create, bool user
) {
    u32 pt = PageDirectoryTable::index(virt);
    auto& entry = table.entries[pt];

    if (!entry.is_present()) {
        if (!create) {
            return nullptr;
        }

        auto* frame = MM->allocate_page_frame();
        if (!frame) {
            return nullptr;
        }

        entry.set_present(true);
        entry.set_writable(true);
        entry.set_user(user);

        entry.set_physical_address(reinterpret_cast<PhysicalAddress>(frame));
        memset(reinterpret_cast<void*>(entry.physical_address() + g_boot_info->hhdm), 0, PAGE_SIZE);
    }

    VirtualAddress address = entry.physical_address() + g_boot_info->hhdm;
    auto* entries = reinterpret_cast<PageTableEntry*>(address);

    return entries + PageTable::index(virt);
}

void PageDirectory::map(VirtualAddress virt, PhysicalAddress phys, PageFlags flags) {
    bool writable = std::has_flag(flags, PageFlags::Write);
    bool user = std::has_flag(flags, PageFlags::User);
    bool cache_disable = std::has_flag(flags, PageFlags::CacheDisable);
    bool no_execute = std::has_flag(flags, PageFlags::NoExecute);

    auto* entry = this->walk_page_table(virt, true, user);
    if (!entry) {
        return;
    }

    entry->set_value(0);
    entry->set_present(true);

    entry->set_writable(writable);
    entry->set_user(user);
    entry->set_cache_disable(cache_disable);
    entry->set_no_execute(no_execute);
    
    entry->set_physical_address(phys);
}

void PageDirectory::unmap(VirtualAddress virt) {
    auto* entry = this->walk_page_table(virt);
    if (!entry) {
        return;
    }

    entry->set_value(0);
    arch::invlpg(virt);
}

PageTableEntry* PageDirectory::get_page_table_entry(VirtualAddress virt) {
    return this->walk_page_table(virt);
}

PageTableEntry const* PageDirectory::get_page_table_entry(VirtualAddress virt) const {
    // I know this is cursed but I also know for sure that walk_page_table won't modify anything so it is safe to do.
    return (const_cast<PageDirectory*>(this))->walk_page_table(virt);
}

PhysicalAddress PageDirectory::get_physical_address(VirtualAddress virt) const {
    auto* entry = this->get_page_table_entry(virt);
    if (!entry) {
        return {};
    }

    return entry->physical_address();
}

bool PageDirectory::is_mapped(VirtualAddress virt) const {
    auto* entry = this->get_page_table_entry(virt);
    return entry && entry->is_present();
}

PhysicalAddress PageDirectory::cr3() const {
    return reinterpret_cast<PhysicalAddress>(m_pml4.entries) - g_boot_info->hhdm;
}

void PageDirectory::switch_to() {
    asm volatile("mov %0, %%cr3" ::"r"(cr3()));
}

PageDirectory* PageDirectory::kernel_page_directory() {
    return &s_kernel_page_directory;
}

void PageDirectory::create_kernel_page_directory(BootInfo const& boot_info, memory::RegionAllocator&) {
    auto& dir = s_kernel_page_directory;
    dir.set_type(Kernel);

    dir.m_pml4 = { reinterpret_cast<PML4Entry*>((u8*)MM->allocate_page_frame() + boot_info.hhdm) };
    memset(dir.m_pml4.entries, 0, PAGE_SIZE);
    
    for (size_t i = 0; i < HHDM_MAPPING_SIZE; i += PAGE_SIZE) {
        VirtualAddress virt = boot_info.hhdm + i;
        dir.map(virt, i, PageFlags::Write);
    }

    // FIXME: This currently maps both the kernel text and data sections as writable
    for (size_t i = 0; i < boot_info.kernel_size; i += PAGE_SIZE) {
        PhysicalAddress phys = boot_info.kernel_physical_base + i;
        VirtualAddress virt = boot_info.kernel_virtual_base + i;
        
        dir.map(virt, phys, PageFlags::Write);
    }
    
    dir.switch_to();
}

}