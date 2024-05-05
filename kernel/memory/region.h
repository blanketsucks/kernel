#pragma once

#include <kernel/common.h>

#include <std/enums.h>

namespace kernel::arch {

class PageDirectory;

}

namespace kernel::memory {

enum class Permissions : u8 {
    None    = 0,
    Read    = 1,
    Write   = 2,
    Execute = 4,
};

MAKE_ENUM_BITWISE_OPS(Permissions);

struct Space {
public:
    Space(size_t size, uintptr_t address) : m_size(size), m_address(address) {}

    size_t size() const { return m_size; }
    uintptr_t address() const { return m_address; }
    
    bool used() const { return m_used; }

    Permissions permissions() const { return m_perms; }

    uintptr_t end() const { return m_address + m_size; }
    bool contains(uintptr_t addr) const { return addr >= m_address && addr < end(); }

    bool is_readable() const { return std::has_flag(m_perms, Permissions::Read); }
    bool is_writable() const { return std::has_flag(m_perms, Permissions::Write); }
    bool is_executable() const { return std::has_flag(m_perms, Permissions::Execute); }

private:
    size_t m_size;
    uintptr_t m_address;

    bool m_used;
    Permissions m_perms;

    Space* next = nullptr;
    Space* prev = nullptr;

    friend class Region;
    friend class MemoryManager;
};

class Region {
public:
    Region() = default;
    Region(uintptr_t start, uintptr_t end, arch::PageDirectory*);

    uintptr_t start() const { return m_start; }
    uintptr_t end() const { return m_end; }

    arch::PageDirectory* page_directory() const { return m_page_directory; }

    bool is_kernel() const { return m_start >= KERNEL_VIRTUAL_BASE ;}
    size_t size() const { return m_end - m_start; }

    Space* base() const { return m_head; }

    bool contains(uintptr_t address) const { return address >= m_start && address < this->end(); }

    Space* reserve(uintptr_t address, size_t size, Permissions perms);

    Space* find_space(uintptr_t address) const;

    Space* find_free(size_t size, bool page_aligned);
    Space* find_free_pages(size_t pages);

    Space* allocate(size_t size, Permissions perms, bool page_aligned = false);
    Space* allocate_at(uintptr_t address, size_t size, Permissions perms);

    void free(uintptr_t address);

private:
    void insert_space_before(Space* space, Space* new_space);
    void insert_space_after(Space* space, Space* new_space);

    uintptr_t m_start;
    uintptr_t m_end;

    arch::PageDirectory* m_page_directory;

    Space* m_head;
};

}