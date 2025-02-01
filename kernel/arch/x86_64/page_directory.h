#pragma once

#include "std/types.h"
#include <kernel/common.h>
#include <kernel/arch/boot_info.h>

#define COMMON_PAGE_METHODS                                                                                         \
    u64 value() const { return m_value; }                                                                           \
    void set_value(u64 value) { m_value = value; }                                                                  \
                                                                                                                    \
    bool is_present() const { return m_value & Present; }                                                           \
    bool is_writable() const { return m_value & Writable; }                                                         \
    bool is_user() const { return m_value & User; }                                                                 \
    bool is_cache_disabled() const { return m_value & CacheDisable; }                                               \
    bool is_no_execute() const { return m_value & NoExecute; }                                                      \
                                                                                                                    \
    PhysicalAddress physical_address() const { return m_value & PHYSICAL_ADDRESS_MASK; }                            \
    void set_physical_address(PhysicalAddress address) { m_value = (m_value & ~PHYSICAL_ADDRESS_MASK) | address; }  \
                                                                                                                    \
    void set_present(bool present) { set_bit(m_value, Present, present); }                                          \
    void set_writable(bool writable) { set_bit(m_value,Writable, writable); }                                       \
    void set_user(bool user) { set_bit(m_value,User, user); }                                                       \
    void set_cache_disable(bool cache_disable) { set_bit(m_value, CacheDisable, cache_disable); }  

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

constexpr u64 PHYSICAL_ADDRESS_MASK = 0xFFFFFFFFFFFFF000ull;

constexpr u32 get_pml4_index(VirtualAddress address) {
    return (address >> 39) & 0x1FF;
}

constexpr u32 get_page_directory_table_index(VirtualAddress address) {
    return (address >> 30) & 0x1FF;
}

constexpr u32 get_page_directory_index(VirtualAddress address) {
    return (address >> 21) & 0x1FF;
}

constexpr u32 get_page_table_index(VirtualAddress address) {
    return (address >> 12) & 0x1FF;
}

constexpr void set_bit(u64& value, u64 bit, bool set) {
    if (set) {
        value |= bit;
    } else {
        value &= ~bit;
    }
}

struct PML4Entry {
public:
    enum Flags : u64 {
        Present = 1 << 0,
        Writable = 1 << 1,
        User = 1 << 2,
        WriteThrough = 1 << 3,
        CacheDisable = 1 << 4,
        Accessed = 1 << 5,
        NoExecute = 0x8000000000000000ull
    };
    
    PhysicalAddress page_directory_table_address() const { return m_value & PHYSICAL_ADDRESS_MASK; }
    void set_page_directory_table_address(PhysicalAddress address) {
        m_value = (m_value & ~PHYSICAL_ADDRESS_MASK) | address;
    }
    
    COMMON_PAGE_METHODS;
private:
    u64 m_value;
};

struct PageDirectoryTableEntry {
public:
    enum Flags : u64 {
        Present = 1 << 0,
        Writable = 1 << 1,
        User = 1 << 2,
        WriteThrough = 1 << 3,
        CacheDisable = 1 << 4,
        Accessed = 1 << 5,
        PageSize = 1 << 7,
        NoExecute = 0x8000000000000000ull
    };

    bool is_huge() const { return m_value & PageSize; }
    void set_huge(bool huge) { set_bit(m_value, PageSize, huge); }

    PhysicalAddress page_directory_address() const { return m_value & PHYSICAL_ADDRESS_MASK; }
    void set_page_directory_address(PhysicalAddress address) {
        m_value = (m_value & ~PHYSICAL_ADDRESS_MASK) | address;
    }

    COMMON_PAGE_METHODS;
private:
    u64 m_value;
};

struct PageDirectoryEntry {
public:
    enum Flags : u64 {
        Present = 1 << 0,
        Writable = 1 << 1,
        User = 1 << 2,
        WriteThrough = 1 << 3,
        CacheDisable = 1 << 4,
        Accessed = 1 << 5,
        PageSize = 1 << 7,
        NoExecute = 0x8000000000000000ull
    };

    bool is_huge() const { return m_value & PageSize; }
    void set_huge(bool huge) { set_bit(m_value, PageSize, huge); }

    PhysicalAddress page_table_address() const { return m_value & PHYSICAL_ADDRESS_MASK; }
    void set_page_table_address(PhysicalAddress address) {
        m_value = (m_value & ~PHYSICAL_ADDRESS_MASK) | address;
    }

    COMMON_PAGE_METHODS;
private:
    u64 m_value;
};

struct PageTableEntry {
public:
    enum Flags : u64 {
        Present = 1 << 0,
        Writable = 1 << 1,
        User = 1 << 2,
        WriteThrough = 1 << 3,
        CacheDisable = 1 << 4,
        Accessed = 1 << 5,
        Dirty = 1 << 6,
        NoExecute = 0x8000000000000000ull
    };

    PageFlags flags() const { return static_cast<PageFlags>(m_value & 0xFFF); }

    bool is_dirty() const { return m_value & Dirty; }
    bool is_accessed() const { return m_value & Accessed; }

    void set_dirty(bool dirty) { set_bit(m_value, Dirty, dirty); }
    void set_accessed(bool accessed) { set_bit(m_value, Accessed, accessed); }

    PhysicalAddress get_physical_address() const { return physical_address(); } // To match the interface of the x86 PageTableEntry

    COMMON_PAGE_METHODS;
private:
    u64 m_value;
};

struct PageTable {
    static u32 index(VirtualAddress address) { return (address >> 12) & 0x1FF; }
    
    PageTableEntry* entries;
};

struct PageDirectoryTable {
    using Next = PageTable;

    PageDirectoryEntry* entries;
    static u32 index(VirtualAddress address) { return (address >> 21) & 0x1FF; }
};

struct PageDirectoryPointerTable {
    using Next = PageDirectoryTable;

    PageDirectoryTableEntry* entries;
    static u32 index(VirtualAddress address) { return (address >> 30) & 0x1FF; }
};

struct PML4 {
    using Next = PageDirectoryPointerTable;

    PML4Entry* entries;
    static u32 index(VirtualAddress address) { return (address >> 39) & 0x1FF; }
};

class PageDirectory {
public:
    enum Type {
        Kernel,
        User
    };

    static void create_kernel_page_directory(arch::BootInfo const&, memory::RegionAllocator& kernel_region_allocator);
    static PageDirectory* create_user_page_directory();

    bool is_user() const { return m_type == Type::User; }
    bool is_kernel() const { return m_type == Type::Kernel; }

    void map(VirtualAddress virt, PhysicalAddress phys, PageFlags flags);
    void unmap(VirtualAddress virt);

    PhysicalAddress get_physical_address(VirtualAddress virt) const;

    bool is_mapped(VirtualAddress virt) const;

    PageTableEntry const* get_page_table_entry(VirtualAddress virt) const;
    PageTableEntry* get_page_table_entry(VirtualAddress virt);

    PhysicalAddress cr3() const;
    void switch_to();

    static PageDirectory* kernel_page_directory();

    PageTableEntry* walk_page_table(VirtualAddress virt, bool create = false, bool user = false);

    template<typename T>
    PageTableEntry* walk_page_table(T table, VirtualAddress virt, bool create = false, bool user = false);

private:
    void set_type(Type type) { m_type = type; }

    PML4 m_pml4;
    Type m_type;
};

template<> PageTableEntry* PageDirectory::walk_page_table(PageDirectoryTable, VirtualAddress, bool create, bool user);

}