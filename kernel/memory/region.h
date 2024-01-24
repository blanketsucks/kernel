#pragma once

#include <kernel/common.h>

#include <std/enums.h>

namespace kernel::memory {

enum class Permissions : u8 {
    None    = 0,
    Read    = 1,
    Write   = 2,
    Execute = 4,
};

MAKE_ENUM_BITWISE_OPS(Permissions);

struct Space {
    Space(size_t size, u32 address) : m_size(size), m_address(address) {}

    size_t size() const { return m_size; }
    u32 address() const { return m_address; }
    bool used() const { return m_used; }
    Permissions permissions() const { return m_perms; }

    u32 end() const { return m_address + m_size; }
    bool contains(u32 addr) const { return addr >= m_address && addr < end(); }

    bool is_readable() const { return std::has_flag(m_perms, Permissions::Read); }
    bool is_writable() const { return std::has_flag(m_perms, Permissions::Write); }
    bool is_executable() const { return std::has_flag(m_perms, Permissions::Execute); }

private:
    size_t m_size;
    u32 m_address;

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
    Region(u32 start, size_t size);

    u32 start() const { return m_start; }
    u32 end() const { return m_start + m_size; }

    bool is_kernel() const { return m_start >= KERNEL_VIRTUAL_BASE ;}
    size_t size() const { return m_size; }

    Space* base() const { return m_head; }

    bool contains(u32 address) const { return address >= m_start && address < end(); }

    Space* reserve(u32 address, size_t size, Permissions perms);

    Space* find_space(u32 address) const;

    Space* find_free(size_t size);
    Space* find_free_pages(size_t pages);

    Space* allocate(size_t size, Permissions perms);
    void free(u32 address);

private:
    void insert_space_before(Space* space, Space* new_space);
    void insert_space_after(Space* space, Space* new_space);

    u32 m_start;
    size_t m_size;

    Space* m_head;
};

}