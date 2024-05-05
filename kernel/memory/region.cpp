#include <kernel/memory/region.h>
#include <kernel/memory/manager.h>

#include <kernel/panic.h>
#include <kernel/vga.h>
#include <kernel/serial.h>

namespace kernel::memory {

Region::Region(uintptr_t start, uintptr_t end, arch::PageDirectory* pd) : m_start(start), m_end(end), m_page_directory(pd) {
    m_head = new Space(this->size(), start);
}

void Region::insert_space_before(Space* space, Space* new_space) {
    new_space->next = space;
    new_space->prev = space->prev;

    if (space->prev) {
        space->prev->next = new_space;
    } else {
        m_head = new_space;
    }

    space->prev = new_space;
}

void Region::insert_space_after(Space* space, Space* new_space) {
    new_space->prev = space;
    new_space->next = space->next;

    if (space->next) {
        space->next->prev = new_space;
    }

    space->next = new_space;
}

Space* Region::reserve(uintptr_t address, size_t size, Permissions perms) {
    Space* space = m_head;
    while (space) {
        if (!space->contains(address)) {
            space = space->next;
            continue;
        }

        // We can't reserve space that's already used
        if (space->m_used) {
            return nullptr;
        }

        // If the space is the same size as the reservation, we can just mark it as used
        if (space->m_size == size) {
            space->m_used = true;
            space->m_perms = perms;

            return space;
        }

        if (space->m_size - (address - space->m_address) < size) {
            return nullptr;
        }

        // Create a new region before `space` if needed
        if (space->m_address < address) {
            auto* before = new Space(address - space->m_address, space->m_address);
            this->insert_space_before(space, before);
        }

        // Create a new region after `space` if needed
        if (space->end() > address + size) {
            auto* after = new Space(space->end() - (address + size), address + size);
            this->insert_space_after(space, after);
        }

        // Mark the space as used
        space->m_used = true;
        space->m_perms = perms;
        space->m_size = size;
        space->m_address = address;

        return space;
    }

    return nullptr;
}

Space* Region::find_space(uintptr_t address) const {
    auto* space = m_head;
    while (space) {
        if (space->m_address == address) {
            return space;
        }

        space = space->next;
    }

    return nullptr;
}

Space* Region::find_free(size_t size, bool page_aligned) {
    auto* space = m_head;
    while (space) {
        if (space->m_used || space->m_size < size) {
            space = space->next;
            continue;
        } else if (space->m_size == size && (!page_aligned || space->m_address % PAGE_SIZE == 0)) {
            return space;
        }

        if (page_aligned && space->m_address % PAGE_SIZE != 0) {
            size_t offset = PAGE_SIZE - (space->m_address % PAGE_SIZE);
            if (offset < size) {
                size -= offset;
                space->m_address += offset;
                space->m_size -= offset;
            }
        }

        auto* new_space = new Space(size, space->m_address);

        space->m_address += size;
        space->m_size -= size;

        this->insert_space_before(space, new_space);
        return new_space;
    }

    return nullptr;
}

Space* Region::find_free_pages(size_t pages) {
    return this->find_free(pages * PAGE_SIZE, true);
}

Space* Region::allocate(size_t size, Permissions perms, bool page_aligned) {
    auto* space = this->find_free(size, page_aligned);
    if (!space) {
        return nullptr;
    }

    space->m_used = true;
    space->m_perms = perms;

    return space;
}

Space* Region::allocate_at(uintptr_t address, size_t size, Permissions perms) {
    return this->reserve(address, size, perms);
}

void Region::free(uintptr_t address) {
    auto* space = this->find_space(address);
    if (!space) {
        return;
    }

    space->m_used = false;
    space->m_perms = Permissions::None;
}

}