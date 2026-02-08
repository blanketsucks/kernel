#include <kernel/sync/spinlock.h>
#include <kernel/arch/processor.h>
#include <kernel/process/threads.h>

namespace kernel {

bool SpinLock::is_locked() {
    return m_lock.load(std::MemoryOrder::Relaxed) != 0;
}

void SpinLock::lock() {
    if (!Processor::are_interrupts_initialized()) {
        return;
    }

    asm volatile("cli");
    while (m_lock.exchange(1, std::MemoryOrder::Acquire) != 0) {
        asm volatile("pause");
    }
}

void SpinLock::unlock() {
    if (!Processor::are_interrupts_initialized()) {
        return;
    }

    m_lock.store(0, std::MemoryOrder::Relaxed);
    asm volatile("sti");
}


}