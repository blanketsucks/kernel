#pragma once

#include <kernel/common.h>
#include <kernel/arch/boot_info.h>

namespace kernel {

enum class PageFlags : u32 {
    None = 0,
    Write = 1 << 1,
    User = 1 << 2,
    CacheDisable = 1 << 3
};

MAKE_ENUM_BITWISE_OPS(PageFlags)

}

namespace kernel::memory {
    class RegionAllocator;
}

namespace kernel::arch {

constexpr u32 get_page_directory_table_index(uintptr_t address) {
    return (address >> 30) & 0x1FF;
}

constexpr u32 get_page_directory_index(uintptr_t address) {
    return (address >> 21) & 0x1FF;
}

constexpr u32 get_page_table_index(uintptr_t address) {
    return (address >> 12) & 0x1FF;
}

union PageDirectoryTableEntry {
    struct {
        bool present : 1;
        bool writable : 1;
        bool user : 1;
        bool write_through : 1;
        bool cache_disable : 1;
        bool accessed : 1;
        bool dirty : 1;
        bool huge_page : 1;
        bool global : 1;
        u64 avl0 : 3;
        bool pat : 1;
        u64 reserved0 : 17;
        u64 page_directory_address : 22;
        u64 avl1 : 7;
        u64 pk : 4;
        bool no_execute : 1;
    } data;

    bool is_present() const { return data.present; }

    void set_present(bool present) { data.present = present; }
    void set_writable(bool writable) { data.writable = writable; }
    void set_user(bool user) { data.user = user; }

    void set_page_directory_address(PhysicalAddress address) { data.page_directory_address = address >> 30; }

    u64 value;
};

union PageDirectoryEntry {
    struct {
        bool present : 1;
        bool writable : 1;
        bool user : 1;
        bool write_through : 1;
        bool cache_disable : 1;
        bool accessed : 1;
        bool dirty : 1;
        bool huge_page : 1;
        bool global : 1;
        u64 avl0 : 3;
        bool pat : 1;
        u64 reserved0 : 8;
        u64 page_table_address : 31;
        u64 avl1 : 7;
        u64 pk : 4;
        bool no_execute : 1;
    } data;

    bool is_present() const { return data.present; }

    void set_present(bool present) { data.present = present; }
    void set_writable(bool writable) { data.writable = writable; }
    void set_user(bool user) { data.user = user; }

    void set_page_table_address(PhysicalAddress address) { data.page_table_address = address >> 21; }

    u64 value;
};

union PageTableEntry {
    struct {
        bool present : 1;
        bool writable : 1;
        bool user : 1;
        bool write_through : 1;
        bool cache_disabled : 1;
        bool accessed : 1;
        bool dirty : 1;
        bool huge_page : 1;
        bool global : 1;
        u64 avl0 : 3;
        u64 physical_address : 40;
        u64 avl1 : 7;
        u64 pk : 4;
        bool no_execute : 1;
    } data;

    bool is_present() const { return data.present; }
    bool is_writable() const { return data.writable; }
    bool is_user() const { return data.user; }
    bool is_cache_disabled() const { return data.cache_disabled; }

    PageFlags flags() const {
        PageFlags flags = PageFlags::None;

        if (is_writable()) {
            flags |= PageFlags::Write;
        } if (is_user()) {
            flags |= PageFlags::User;
        } if (is_cache_disabled()) {
            flags |= PageFlags::CacheDisable;
        }

        return flags;
    }

    PhysicalAddress get_physical_address() const { return data.physical_address << 12; }

    void set_present(bool present) { data.present = present; }
    void set_writable(bool writable) { data.writable = writable; }
    void set_user(bool user) { data.user = user; }
    void set_cache_disable(bool cache_disable) { data.cache_disabled = cache_disable; }

    void set_physical_address(PhysicalAddress address) { data.physical_address = address >> 12; }

    u64 value;
};

class PageTable {
public:
    PageTable() = default;

    static PageTable* create();

    PageTableEntry& operator[](size_t index) { return this->at(index); }
    PageTableEntry const& operator[](size_t index) const { return this->at(index); }

    PageTableEntry& at(size_t index) { return m_entries[index]; }
    PageTableEntry const& at(size_t index) const { return m_entries[index]; }

    PageTableEntry* entries() { return m_entries; }
    VirtualAddress address() const { return reinterpret_cast<VirtualAddress>(m_entries); }

private:
    PageTableEntry* m_entries;

    friend class PageDirectory;
};

class PageDirectoryTable {
public:
    PageDirectoryTable() = default;

    static PageDirectoryTable* create();

    VirtualAddress address() const { return reinterpret_cast<VirtualAddress>(m_entries); }

    PhysicalAddress get_physical_address(VirtualAddress virt) const;

    PageTable* create_page_table(u32 pd, bool user);
    PageTable* get_page_table(u32 pd);

private:
    PageDirectoryEntry* m_entries;
    PageTable* m_tables[512];

    friend class PageDirectory;
};

// This PageDirectory class will represent the PML4 table
class PageDirectory {
public:
    static void create_kernel_page_directory(arch::BootInfo const&, memory::RegionAllocator& kernel_region_allocator);
    static PageDirectory* create_user_page_directory() { return nullptr; }

    void map(VirtualAddress virt, PhysicalAddress phys, PageFlags flags);
    void unmap(VirtualAddress virt);

    PhysicalAddress get_physical_address(VirtualAddress virt) const;

    bool is_mapped(VirtualAddress virt) const { return false; }

    PageTable* create_page_table(u32 pdt, u32 pd, bool user);
    PageTable* get_page_table(u32 pdt, u32 pd);

    PageTableEntry const* get_page_table_entry(VirtualAddress virt) const;
    PageTableEntry* get_page_table_entry(VirtualAddress virt);

    PhysicalAddress cr3() const;
    void switch_to();

    static PageDirectory* kernel_page_directory();

private:
    PageDirectoryTableEntry* m_entries;
    PageDirectoryTable* m_tables[512];
};

}