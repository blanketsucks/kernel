#include <kernel/memory/region.h>
#include <kernel/memory/manager.h>
#include <kernel/fs/file.h>
#include <kernel/arch/cpu.h>

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

RegionAllocator::RegionAllocator(
    const Range& range, Region* head, arch::PageDirectory* page_directory
) : m_range(range), m_head(head), m_page_directory(page_directory) {}

RefPtr<RegionAllocator> RegionAllocator::clone(arch::PageDirectory* page_directory) {
    ScopedSpinLock lock(m_lock);

    auto allocator = RegionAllocator::create(m_range, m_head->clone(), page_directory);

    Region* prev = allocator->m_head;
    Region* region = m_head->next;

    if (prev->used()) {
        this->map_into(page_directory, prev);
    }

    while (region) {
        auto* cloned = region->clone();

        prev->next = cloned;
        cloned->prev = prev;
    
        prev = cloned;
        if (region->used()) {
            this->map_into(page_directory, region);
        }

        region = region->next;
    }

    return allocator;
}

void RegionAllocator::map_into(arch::PageDirectory* page_directory, Region* region) const {
    size_t pages = region->size() / PAGE_SIZE;

    bool is_shared = region->is_shared();
    VirtualAddress base = region->base();

    for (size_t i = 0; i < pages; i++) {
        VirtualAddress address = base.offset(i * PAGE_SIZE);
        auto* entry = m_page_directory->get_page_table_entry(address);

        if (!entry) {
            continue;
        }
        
        PhysicalAddress pa = entry->get_physical_address();
        PhysicalPage* page = MM->get_physical_page(pa);

        if (!page) {
            continue;
        }

        if (!is_shared) {            
            page->flags |= PhysicalPage::CoW;
            entry->set_writable(false);
        }

        PageFlags flags = PageFlags::User;
        if (entry->is_writable()) {
            flags |= PageFlags::Write;
        }

        if (entry->is_no_execute()) {
            flags |= PageFlags::NoExecute;
        }
        
        page->ref_count++;
        page_directory->map(address, pa, flags);
    }
}

Region* RegionAllocator::insert_before(Region* region, Region* new_region) {
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

Region* RegionAllocator::insert_after(Region* region, Region* new_region) {
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
            auto* before = Region::create(region->base(), address - region->base());
            this->insert_before(region, before);
        }

        // Create a new region after `region` if needed
        if (region->end() > address + size) {
            auto* after = Region::create(address.offset(size), region->end() - (address + size));
            this->insert_after(region, after);
        }

        // Mark the region as used
        region->m_used = true;
        region->m_prot = prot;

        region->set_range({ address, size });

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

        auto* new_region = Region::create(region->base(), size);
        region->set_range({ region->offset_by(size), region->size() - size });

        this->insert_before(region, new_region);
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

Region* RegionAllocator::create_file_backed_region(fs::File* file, size_t size, VirtualAddress hint) {
    Region* region = nullptr;
    if (hint) {
        region = this->allocate_at(hint, size, PROT_READ | PROT_WRITE);
    } else {
        region = this->allocate(size, PROT_READ | PROT_WRITE);
    }

    if (!region) {
        return nullptr;
    }

    region->m_file = file;
    return region;
}

}