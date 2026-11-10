#include <kernel/sync/spinlock.h>
#include <kernel/arch/processor.h>
#include <kernel/process/threads.h>

namespace kernel {

bool SpinLock::is_locked() {
    return m_lock.load(std::MemoryOrder::Relaxed) != 0;
}

InterruptState SpinLock::lock() {
    if (!Processor::are_interrupts_initialized()) {
        return InterruptState::Disabled;
    }

    InterruptState saved = Processor::interrupt_state();
    Processor::disable_interrupts();

    while (m_lock.exchange(1, std::MemoryOrder::Acquire) != 0) {
        asm volatile("pause");
    }

    return saved;
}

void SpinLock::unlock(InterruptState saved) {
    if (!Processor::are_interrupts_initialized()) {
        return;
    }

    m_lock.store(0, std::MemoryOrder::Release);
    Processor::set_interrupt_state(saved);
}


}
