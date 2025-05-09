#pragma once

#include <kernel/common.h>

#include <std/atomic.h>

namespace kernel {

class SpinLock {
public:
    SpinLock() = default;

    SpinLock(const SpinLock&) = delete;
    SpinLock(SpinLock&&) = delete;

    SpinLock& operator=(const SpinLock&) = delete;
    SpinLock& operator=(SpinLock&&) = delete;

    bool is_locked();

    void lock();
    void unlock();

private:
    std::Atomic<u8> m_lock { 0 };
};

template<typename Lock>
class ScopedSpinLock {
public:
    explicit ScopedSpinLock(Lock& lock) : m_lock(lock) {
        m_lock.lock();
    }

    ~ScopedSpinLock() {
        m_lock.unlock();
    }

private:
    Lock& m_lock;
};

}