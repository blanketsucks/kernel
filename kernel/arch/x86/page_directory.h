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

union PageDirectoryEntry {
    struct PACKED {
        u32 present : 1;
        u32 writable : 1;
        u32 user : 1;
        u32 write_through : 1;
        u32 cache_disabled : 1;
        u32 accessed : 1;
        u32 avl0 : 1;
        u32 page_size : 1;
        u32 avl1 : 4;
        u32 page_table_address : 20;
    } data;

    u32 value;

    bool is_present() const { return data.present; }
    u32 get_page_table_address() { return data.page_table_address << 12; }

    void set_present(bool present) { data.present = present; }
    void set_writable(bool writable) { data.writable = writable; }
    void set_user(bool user) { data.user = user; }
    void set_page_table_address(u32 addr) { data.page_table_address = addr >> 12; }
};

union PageTableEntry {
    struct PACKED {
        u32 present : 1;
        u32 writable : 1;
        u32 user : 1;
        u32 write_through : 1;
        u32 cache_disabled : 1;
        u32 accessed : 1;
        u32 dirty : 1;
        u32 pat : 1;
        u32 global : 1;
        u32 available : 3;
        u32 physical_address : 20;
    } data;

    u32 value;

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
    void set_physical_address(u32 addr) { data.physical_address = addr >> 12; }
};

constexpr u32 get_page_directory_index(VirtualAddress virt) { return virt >> 22; }
constexpr u32 get_page_table_index(VirtualAddress virt) { return virt >> 12 & 0x3FF; }

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
    friend class PageDirectory;

    PageTableEntry* m_entries;
};

class PageDirectory {
public:
    PageDirectory();
    
    enum Type {
        User,
        Kernel
    };

    static PageDirectory* create_user_page_directory();
    static void create_kernel_page_directory(BootInfo const&, memory::RegionAllocator& kernel_region_allocator);

    void clone_into(PageDirectory* other) const;
    PageDirectory* clone() const;

    Type type() const { return m_type; }
    bool is_kernel() const { return m_type == Kernel; }

    void map(VirtualAddress virt, PhysicalAddress phys, PageFlags flags);
    void unmap(VirtualAddress virt);

    PageTableEntry const* get_page_table_entry(VirtualAddress virt) const;
    PhysicalAddress get_physical_address(VirtualAddress virt) const;

    bool is_mapped(VirtualAddress virt) const;

    void clear();

    PageTable* create_page_table(u32 pd, bool user);

    PageTable* get_page_table(u32 pd);
    PageTable const* get_page_table(u32 pd) const;

    PhysicalAddress cr3() const;
    void switch_to();

    static PageDirectory* kernel_page_directory();
private:
    PageDirectory(Type type) : m_type(type) {}

    PageDirectoryEntry* m_entries;
    PageTable* m_tables[1024];

    Type m_type;
};

void invlpg(void* addr);

}