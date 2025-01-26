#include <kernel/memory/physical.h>
#include <kernel/memory/manager.h>
#include <kernel/memory/region.h>

#include <kernel/vga.h>
#include <kernel/serial.h>
#include <std/string.h>

namespace kernel::memory {

static PhysicalMemoryManager s_instance = PhysicalMemoryManager();

PhysicalMemoryManager* PhysicalMemoryManager::instance() {
    return &s_instance;
}

void PhysicalMemoryManager::init(arch::BootInfo const& boot_info) {
    serial::printf("Memory map:\n");

    for (u32 i = 0; i < boot_info.mmap.count; i++) {
        auto& entry = boot_info.mmap.entries[i];

        size_t size = std::align_down(entry.length, static_cast<u64>(PAGE_SIZE));
        uintptr_t address = std::align_up(entry.base, static_cast<u64>(PAGE_SIZE));

        serial::printf("  Address: %#x, Size: %#x, Type: %d\n", address, size, static_cast<u32>(entry.type));
        if (entry.type != arch::MemoryType::Available) {
            continue;
        }

        for (u32 j = 0; j < size; j += PAGE_SIZE) {
            s_instance.m_physical_frames.push(address + j);
        }

        s_instance.m_total_usable_memory += size;
    }

    s_instance.m_initialized = true;
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