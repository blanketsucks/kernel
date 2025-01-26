#include <kernel/memory/region.h>
#include <kernel/memory/manager.h>
#include <kernel/fs/file.h>
#include <kernel/panic.h>
#include <kernel/posix/sys/mman.h>

#include <std/format.h>

namespace kernel::memory {

Region* Region::clone() const {
    auto* region = new Region(m_range);

    region->m_used = m_used;
    region->m_prot = m_prot;
    region->m_file = m_file;

    return region;
}

RegionAllocator::RegionAllocator(const Range& range, arch::PageDirectory* page_directory) : m_range(range), m_page_directory(page_directory) {
    m_head = new Region(range);
}

RegionAllocator RegionAllocator::clone_with_page_directory(arch::PageDirectory* page_directory) const {
    RegionAllocator allocator = {};

    allocator.m_page_directory = page_directory;
    allocator.m_head = nullptr;

    Region* prev = nullptr;
    this->for_each_region([&](Region* region) {
        if (!region->used()) {
            if (!prev) {
                prev = allocator.m_head = region->clone();
            } else {
                prev = allocator.insert_region_before(prev, region->clone());
            }

            return;
        }

        this->map_into(page_directory, region);
        if (!prev) {
            prev = allocator.m_head = region->clone();
        } else {
            prev = allocator.insert_region_before(prev, region->clone());
        }
    });

    return allocator;
}

void RegionAllocator::map_into(arch::PageDirectory* page_directory, Region* region) const {
    size_t pages = region->size() / PAGE_SIZE;
    for (size_t i = 0; i < pages; i++) {
        VirtualAddress address = region->base() + i * PAGE_SIZE;
        auto* entry = m_page_directory->get_page_table_entry(address);

        if (!entry) {
            continue;
        }

        PhysicalAddress dst = entry->get_physical_address();
        if (!region->is_shared()) {
            void* frame = MM->allocate_physical_frame();
            MM->copy_physical_memory(frame, reinterpret_cast<void*>(dst), PAGE_SIZE);
    
            dst = reinterpret_cast<PhysicalAddress>(frame);
        }

        page_directory->map(address, dst, entry->flags());
    }
}

Region* RegionAllocator::insert_region_before(Region* region, Region* new_region) {
    new_region->next = region;
    new_region->prev = region->prev;

    if (region->prev) {
        region->prev->next = new_region;
    } else {
        m_head = new_region;
    }

    region->prev = new_region;
    return new_region;
}

Region* RegionAllocator::insert_region_after(Region* region, Region* new_region) {
    new_region->prev = region;
    new_region->next = region->next;

    if (region->next) {
        region->next->prev = new_region;
    }

    region->next = new_region;
    return new_region;
}

Region* RegionAllocator::allocate_at(VirtualAddress address, size_t size, int prot) {
    Region* region = m_head;
    while (region) {
        if (!region->contains(address)) {
            region = region->next;
            continue;
        }

        // We can't allocate at a region that's already used
        if (region->used()) {
            return nullptr;
        }

        // If the region is the same size as the allocation, we can just mark it as used
        if (region->size() == size) {
            region->m_used = true;
            region->m_prot = prot;

            return region;
        }

        if (region->size() - (address - region->base()) < size) {
            return nullptr;
        }

        // Create a new region before `region` if needed
        if (region->base() < address) {
            auto* before = new Region({ region->base(), address - region->base() });
            this->insert_region_before(region, before);
        }

        // Create a new region after `region` if needed
        if (region->end() > address + size) {
            auto* after = new Region({ address + size, region->end() - (address + size) });
            this->insert_region_after(region, after);
        }

        // Mark the region as used
        region->m_used = true;
        region->m_prot = prot;

        region->set_size(size);
        region->set_base(address);

        m_usage += size;
        return region;
    }

    return nullptr;
}

Region* RegionAllocator::find_region(VirtualAddress address, bool contains) const {
    auto* region = m_head;
    while (region) {
        if (contains && region->contains(address)) {
            return region;
        } else if (region->base() == address) {
            return region;
        }

        region = region->next;
    }

    return nullptr;
}

Region* RegionAllocator::find_free_region(size_t size) {
    auto* region = m_head;
    while (region) {
        if (region->used() || region->size() < size) {
            region = region->next;
            continue;
        } else if (region->size() == size) {
            return region;
        }

        auto* new_region = new Region({ region->base(), size });

        region->set_base(region->base() + size);
        region->set_size(region->size() - size);

        this->insert_region_before(region, new_region);
        return new_region;
    }

    return nullptr;
}

Region* RegionAllocator::allocate(size_t size, int prot) {
    ASSERT(size % PAGE_SIZE == 0, "size must be page aligned");
    auto* region = this->find_free_region(size);

    if (!region) {
        return nullptr;
    }

    region->m_used = true;
    region->m_prot = prot;

    m_usage += size;
    return region;
}

void RegionAllocator::reserve(VirtualAddress address, size_t size, int prot) {
    this->allocate_at(address, size, prot);
}

void RegionAllocator::free(VirtualAddress address) {
    auto* region = this->find_region(address);
    if (!region) {
        return;
    }

    this->free(region);
}

void RegionAllocator::free(Region* region) {
    region->m_used = false;
    region->m_prot = PROT_NONE;

    m_usage -= region->size();
}

Region* RegionAllocator::create_file_backed_region(fs::File* file, size_t size) {
    auto* region = this->allocate(size, PROT_READ);
    if (!region) {
        return nullptr;
    }

    region->m_file = file;
    return region;
}

}