#include <kernel/memory/physical.h>
#include <kernel/memory/manager.h>
#include <kernel/memory/region.h>

#include <std/format.h>
#include <std/string.h>

namespace kernel::memory {

static PhysicalMemoryManager s_instance = PhysicalMemoryManager();

PhysicalMemoryManager* PhysicalMemoryManager::instance() {
    return &s_instance;
}

void PhysicalMemoryManager::init(arch::BootInfo const& boot_info) {
    dbgln("Memory map:");

    for (u32 i = 0; i < boot_info.mmap.count; i++) {
        auto& entry = boot_info.mmap.entries[i];

        size_t size = std::align_down(entry.length, static_cast<u64>(PAGE_SIZE));
        PhysicalAddress address = std::align_up(entry.base, static_cast<u64>(PAGE_SIZE));

        PhysicalAddress end = entry.base + size;
        dbgln("  {:#p} - {:#p}: {}", address, end, arch::memory_type_to_string(entry.type));
        if (entry.type != arch::MemoryType::Available || size < PAGE_SIZE) {
            continue;
        }

        for (size_t i = 0; i < size; i += PAGE_SIZE) {
            s_instance.m_physical_frames.push(address + i);
        }

        s_instance.m_total_usable_memory += size;
    }

    s_instance.m_initialized = true;

    size_t pages = s_instance.m_total_usable_memory / PAGE_SIZE;

    dbgln("Total usable pages: {}", pages);
    dbgln("Total usable memory: {} MB\n", s_instance.m_total_usable_memory / MB);
}

void* PhysicalMemoryManager::allocate() {
    if (!this->is_initialized() || m_physical_frames.empty()) {
        return nullptr;
    }

    m_allocations++;
    return reinterpret_cast<void*>(m_physical_frames.pop());
}

bool PhysicalMemoryManager::is_allocated(void* frame) const {
    if (!this->is_initialized()) {
        return false;
    }

    auto iterator = m_physical_frames.find(reinterpret_cast<uintptr_t>(frame));
    return iterator == m_physical_frames.end();
}

ErrorOr<void> PhysicalMemoryManager::free(void* frame) {
    if (!this->is_initialized()) return {};

    if (!this->is_allocated(frame)) {
        return Error(EINVAL);
    }

    m_physical_frames.push(reinterpret_cast<uintptr_t>(frame));
    m_allocations--;

    return {};
}

}