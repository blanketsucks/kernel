#pragma once

#include <kernel/common.h>
#include <kernel/posix/sys/mman.h>
#include <kernel/sync/spinlock.h>

#include <std/enums.h>
#include <std/memory.h>
#include <std/string.h>

namespace kernel::arch {
    class PageDirectory;
}

namespace kernel::fs {
    class File;
}

namespace kernel::memory {

struct Range {
public:
    Range() = default;
    Range(VirtualAddress base, size_t size) : m_base(base), m_size(size) {}

    VirtualAddress base() const { return m_base; }
    size_t size() const { return m_size; }

    VirtualAddress end() const { return m_base.offset(m_size); }

    bool contains(VirtualAddress addr) const { return addr >= m_base && addr < end(); }
    bool contains(Range const& other) const {
        return other.base() >= m_base && other.end() <= end();
    }

private:
    VirtualAddress m_base;
    size_t m_size;

    friend class Region;
};

class Region {
public:
    Region(const Range& range) : m_range(range), next(nullptr), prev(nullptr) {}
    Region() = default;

    static Region* create(VirtualAddress base, size_t size) {
        return new Region({ base, size });
    }

    Region* clone() const;

    Range const& range() const { return m_range; }

    String const& name() const { return m_name; }
    void set_name(String name) { m_name = move(name); }

    fs::File* file() { return m_file; }
    fs::File const* file() const { return m_file; }

    void set_file(fs::File* file) { m_file = file; }

    off_t offset() const { return m_offset; }

    bool is_file_backed() const { return m_file != nullptr; }

    size_t size() const { return m_range.size(); }

    VirtualAddress base() const { return m_range.base(); }
    VirtualAddress end() const { return m_range.end(); }

    VirtualAddress offset_by(size_t offset) const {
        return m_range.base().offset(offset);
    }

    size_t offset_in(VirtualAddress address) const {
        return address - m_range.base();
    }
    
    bool used() const { return m_used; }

    int prot() const { return m_prot; }
    
    bool contains(VirtualAddress address) const { return m_range.contains(address); }
    
    bool is_readable() const { return m_prot & PROT_READ; }
    bool is_writable() const { return m_prot & PROT_WRITE; }
    bool is_executable() const { return m_prot & PROT_EXEC; }
    
    bool is_shared() const { return m_shared; }
    void set_shared(bool shared) { m_shared = shared; }
    
    // true if the underlying physical memory that is pointed by this region is managed by the kernel.
    bool is_kernel_managed() const { return m_kernel_managed; }

    void set_kernel_managed(bool kernel_managed) { m_kernel_managed = kernel_managed; }
    void set_range(const Range& range) { m_range = range; }
    void set_prot(int prot) { m_prot = prot; }
    void set_offset(off_t offset) { m_offset = offset; }

private:
    Range m_range;
    String m_name;

    bool m_used = false;
    bool m_shared = false;
    bool m_kernel_managed = false;

    int m_prot = 0;

    fs::File* m_file = nullptr;
    off_t m_offset = 0;

    Region* next = nullptr;
    Region* prev = nullptr;

    friend class RegionAllocator;
    friend class MemoryManager;
};

class RegionAllocator {
public:
    RegionAllocator() = default;
    RegionAllocator(const Range& range, arch::PageDirectory*);
    RegionAllocator(const Range& range, Region* head, arch::PageDirectory* page_directory);

    static RefPtr<RegionAllocator> create(const Range& range, arch::PageDirectory* page_directory) {
        return RefPtr<RegionAllocator>(new RegionAllocator(range, page_directory));
    }

    static RefPtr<RegionAllocator> create(const Range& range, Region* head, arch::PageDirectory* page_directory) {
        return RefPtr<RegionAllocator>(new RegionAllocator(range, head, page_directory));
    }

    RefPtr<RegionAllocator> clone(arch::PageDirectory*);

    Range const& range() const { return m_range; }
    arch::PageDirectory* page_directory() const { return m_page_directory; }

    size_t usage() const { return m_usage; }

    Region* allocate(size_t size, int prot);
    Region* allocate_at(VirtualAddress address, size_t size, int prot);

    void reserve(VirtualAddress address, size_t size, int prot);

    void free(VirtualAddress address);
    void free(Region* region);

    Region* create_file_backed_region(fs::File* file, size_t size, VirtualAddress hint = {});

    Region* find_region(void* address, bool contains = false) const {
        return find_region(VirtualAddress(address), contains);
    }
    
    Region* find_region(VirtualAddress address, bool contains = false) const;

    template<typename T>
    void for_each_region(T&& callback) {
        for (auto* region = m_head; region; region = region->next) {
            callback(region);
        }
    }

    Region* insert_before(Region* region, Region* new_region);
    Region* insert_after(Region* region, Region* new_region);

private:
    void map_into(arch::PageDirectory*, Region* region) const;

    Region* find_free_region(size_t size);

    Range m_range;
    Region* m_head = nullptr;

    size_t m_usage = 0;
    arch::PageDirectory* m_page_directory;

    SpinLock m_lock;
};


}