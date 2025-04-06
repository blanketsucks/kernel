#include <kernel/memory/pmm.h>
#include <kernel/memory/manager.h>
#include <kernel/memory/region.h>

#include <std/format.h>
#include <std/string.h>

namespace kernel::memory {

PhysicalRegion::PhysicalRegion(PhysicalAddress base, size_t size) : m_base(base), m_size(size), m_free_pages(size / PAGE_SIZE) {
    m_bitmap = std::Bitmap::create(size / PAGE_SIZE);
    m_bitmap.clear();
}

ErrorOr<void*> PhysicalRegion::allocate() {
    size_t index = m_bitmap.find_first_unset();
    if (index == m_bitmap.size()) {
        return Error(ENOMEM);
    }

    m_bitmap.set(index, true);
    m_free_pages--;

    return reinterpret_cast<void*>(m_base + index * PAGE_SIZE);
}

ErrorOr<void*> PhysicalRegion::allocate_contiguous(size_t count) {
    if (count == 1) {
        return this->allocate();
    } else if (count > this->page_count()) {
        return Error(ENOMEM);
    }

    for (size_t i = 0; i < m_bitmap.size(); i++) {
        if (m_bitmap.get(i)) {
            continue;
        }

        size_t j = i;
        for (; j < i + count; j++) {
            if (m_bitmap.get(j)) {
                break;
            }
        }

        if (j == i + count) {
            for (size_t k = i; k < j; k++) {
                m_bitmap.set(k, true);
            }

            return reinterpret_cast<void*>(m_base + i * PAGE_SIZE);
        }

        i = j;
    }

    m_free_pages -= count;
    return Error(ENOMEM);
}

ErrorOr<void> PhysicalRegion::free(void* frame, size_t count) {
    size_t index = (reinterpret_cast<PhysicalAddress>(frame) - m_base) / PAGE_SIZE;
    for (size_t i = 0; i < count; i++) {
        m_bitmap.set(index + i, false);
    }

    m_free_pages += count;
    return {};
}

PhysicalMemoryManager* PhysicalMemoryManager::create(arch::BootInfo const& boot_info) {
    PhysicalMemoryManager* pmm = new PhysicalMemoryManager();
    pmm->init(boot_info);
    
    return pmm;
}

void PhysicalMemoryManager::init(arch::BootInfo const& boot_info) {
    dbgln("Memory map:");
    for (size_t i = 0; i < boot_info.mmap.count; i++) {
        auto& entry = boot_info.mmap.entries[i];

        size_t size = std::align_down(entry.length, static_cast<u64>(PAGE_SIZE));
        PhysicalAddress address = std::align_up(entry.base, static_cast<u64>(PAGE_SIZE));

        PhysicalAddress end = entry.base + size;
        dbgln("  {:#p} - {:#p}: {}", address, end, arch::memory_type_to_string(entry.type));
        if (entry.type != arch::MemoryType::Available || size < PAGE_SIZE) {
            continue;
        }

        PhysicalRegion region(address, size);
        m_physical_regions.append(region);

        m_total_usable_memory += size;
    }

    m_initialized = true;
    m_total_pages = m_total_usable_memory / PAGE_SIZE;

    // FIXME: This specific call crashes the kernel down the line for some reason
    // this->fill_preframe_buffer();

    dbgln("Total usable pages: {}", m_total_pages);
    dbgln("Total usable memory: {} MB\n", m_total_usable_memory / MB);
}

void PhysicalMemoryManager::fill_preframe_buffer() {
    if (!m_frames.empty()) {
        return;
    }

    size_t remaining = PREFRAME_COUNT;
    for (auto& region : m_physical_regions) {
        if (region.free_pages() < remaining) {
            continue;
        }

        auto result = region.allocate_contiguous(remaining);
        if (result.is_err()) {
            continue;
        }

        PhysicalAddress address = reinterpret_cast<PhysicalAddress>(result.value());
        for (size_t i = 0; i < remaining; i++) {
            m_frames.push(address + i * PAGE_SIZE);
        }

        break;
    }
}

ErrorOr<void*> PhysicalMemoryManager::allocate() {
    if (!this->is_initialized()) {
        return Error(ENXIO);
    }

    for (auto& region : m_physical_regions) {
        auto result = region.allocate();
        if (result.is_ok()) {
            m_allocations++;
            return result.value();
        }
    }

    return Error(ENOMEM);
}

ErrorOr<void*> PhysicalMemoryManager::allocate_contiguous(size_t count) {
    if (!this->is_initialized()) {
        return Error(ENXIO);
    }
    
    for (auto& region : m_physical_regions) {
        auto result = region.allocate_contiguous(count);
        if (result.is_ok()) {
            m_allocations += count;
            return result.value();
        }
    }

    return Error(ENOMEM);
}

ErrorOr<void> PhysicalMemoryManager::free(void* frame, size_t count) {
    if (!this->is_initialized()) {
        return Error(ENXIO);
    }

    for (auto& region : m_physical_regions) {
        if (region.contains(reinterpret_cast<PhysicalAddress>(frame))) {
            m_allocations -= count;
            return region.free(frame, count);
        }
    }

    return {};
}

}