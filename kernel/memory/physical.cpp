#include "kernel/multiboot.h"
#include <kernel/memory/physical.h>
#include <kernel/memory/mm.h>

#include <kernel/vga.h>
#include <kernel/serial.h>
#include <std/string.h>

namespace kernel::memory {

static PhysicalMemoryManager s_instance = PhysicalMemoryManager();

PhysicalMemoryManager* PhysicalMemoryManager::instance() {
    return &s_instance;
}

void PhysicalMemoryManager::init(multiboot_info_t* header) {
    serial::printf("Memory map:\n");
    u32 frames = 0;

    for (u32 i = 0; i < header->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* entry = reinterpret_cast<multiboot_memory_map_t*>(header->mmap_addr + KERNEL_VIRTUAL_BASE + i);

        u32 size = std::align_down((u32)entry->len, PAGE_SIZE);
        u32 address = std::align_up((u32)entry->addr, PAGE_SIZE);

        serial::printf("  Address: %#x, Size: %#x, Type: %d\n", address, size, entry->type);
        if (entry->type != MULTIBOOT_MEMORY_AVAILABLE) {
            continue;
        }

        frames += size / PAGE_SIZE;
        s_instance.m_total_usable_ram += size;
    }

    s_instance.m_physical_frames.reserve(frames);
    for (u32 i = 0; i < header->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t* entry = reinterpret_cast<multiboot_memory_map_t*>(header->mmap_addr + KERNEL_VIRTUAL_BASE + i);

        u32 size = std::align_down((u32)entry->len, PAGE_SIZE);
        u32 address = std::align_up((u32)entry->addr, PAGE_SIZE);

        if (entry->type != MULTIBOOT_MEMORY_AVAILABLE) {
            continue;
        }

        for (u32 j = 0; j < size; j += PAGE_SIZE) {
            s_instance.m_physical_frames.push(address + j);
        }
    }

    s_instance.m_initialized = true;
}

ErrorOr<void*> PhysicalMemoryManager::allocate() {
    if (!this->is_initialized()) return nullptr;

    if (m_physical_frames.empty()) {
        return Error(ENOMEM);
    }

    m_allocations++;
    return { reinterpret_cast<void*>(m_physical_frames.pop()) };
}

bool PhysicalMemoryManager::is_allocated(void* frame) const {
    if (!this->is_initialized()) {
        return false;
    }

    auto iterator = m_physical_frames.find(reinterpret_cast<u32>(frame));
    return iterator == m_physical_frames.end();
}

ErrorOr<void> PhysicalMemoryManager::free(void* frame) {
    if (!this->is_initialized()) return {};

    if (!this->is_allocated(frame)) {
        return Error(EINVAL);
    }

    m_physical_frames.push(reinterpret_cast<u32>(frame));
    m_allocations--;

    return {};
}

}