#include <kernel/memory/region.h>
#include <kernel/memory/mm.h>

#include <kernel/panic.h>
#include <kernel/vga.h>
#include <kernel/serial.h>

namespace kernel::memory {

Region::Region(u32 start, size_t size) : m_start(start), m_size(size) {
    m_head = new Space(size, start);
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

Space* Region::reserve(u32 address, size_t size, Permissions perms) {
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

Space* Region::find_space(u32 address) const {
    auto* space = m_head;
    while (space) {
        if (space->m_address == address) {
            return space;
        }

        space = space->next;
    }

    return nullptr;
}

Space* Region::find_free(size_t size) {
    auto* space = m_head;
    while (space) {
        serial::printf("space.size = %d, space.used = %d\n", space->m_size, space->m_used);
        if (space->m_used || space->m_size < size) {
            space = space->next;
            continue;
        } else if (space->m_size == size) {
            return space;
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
    return this->find_free(pages * PAGE_SIZE);
}

Space* Region::allocate(size_t size, Permissions perms) {
    auto* space = this->find_free(size);
    if (!space) {
        return nullptr;
    }

    space->m_used = true;
    space->m_perms = perms;

    return space;
}

void Region::free(u32 address) {
    auto* space = this->find_space(address);
    if (!space) {
        return;
    }

    space->m_used = false;
    space->m_perms = Permissions::None;
}

}