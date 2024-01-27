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

void PhysicalMemoryManager::init(u32 mem_lower, u32 mem_upper) {
    u32 frames = (mem_upper + mem_lower) * 1024 / PAGE_SIZE;
    s_instance.m_physical_frames.reserve(frames);

    for (u32 i = 0; i < frames; i++) {
        s_instance.m_physical_frames.push(i * PAGE_SIZE);
    }

    s_instance.m_upper_address = s_instance.m_physical_frames.top();

    s_instance.frames = frames;
    s_instance.initialized = true;
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
    } else if ((u32)frame > m_upper_address) {
        return Error(EINVAL);
    }

    m_physical_frames.push(reinterpret_cast<u32>(frame));
    m_allocations--;

    return {};
}

}