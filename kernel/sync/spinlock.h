#pragma once

#include <kernel/common.h>
#include <kernel/posix/sys/types.h>
#include <kernel/arch/processor.h>

#include <std/atomic.h>

namespace kernel {

class SpinLock {
public:
    SpinLock() = default;

    NO_COPY(SpinLock)
    NO_MOVE(SpinLock)

    bool is_locked();

    InterruptState lock();
    void unlock(InterruptState previous_interrupt_state);

private:
    std::Atomic<u8> m_lock { 0 };
};

class SpinLockGuard {
public:
    NO_COPY(SpinLockGuard);
    NO_MOVE(SpinLockGuard);

    SpinLockGuard(SpinLock& lock) : m_lock(&lock) {
        m_interrupt_state = lock.lock();
    }

    ~SpinLockGuard() {
        m_lock->unlock(m_interrupt_state);
    }

private:
    SpinLock* m_lock;
    InterruptState m_interrupt_state = InterruptState::Disabled;
};

}
