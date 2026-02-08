#include <kernel/sync/mutex.h>
#include <kernel/process/threads.h>

namespace kernel {

void Mutex::lock() {
    auto* thread = Thread::current();
    if (!thread) {
        return;
    }

    pid_t owner = m_owner.load(std::MemoryOrder::Acquire);
    if (owner == thread->id()) {
        m_recursion_count.fetch_add(1, std::MemoryOrder::Relaxed);
        return;
    }

    pid_t expected = -1;
    while (!m_owner.compare_exchange_strong(expected, thread->id(), std::MemoryOrder::Acquire)) {
        expected = -1;
        asm volatile("pause");
    }

    m_recursion_count.store(1, std::MemoryOrder::Relaxed);
}

void Mutex::unlock() {
    auto* thread = Thread::current();
    if (!thread) {
        return;
    }

    pid_t owner = m_owner.load(std::MemoryOrder::Acquire);
    if (owner != thread->id()) {
        return;
    }

    if (m_recursion_count.fetch_sub(1, std::MemoryOrder::Relaxed) != 1) {
        return;
    }

    m_owner.store(-1, std::MemoryOrder::Release);
}

}