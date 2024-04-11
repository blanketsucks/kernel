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

    void acquire();
    void release();

private:
    std::Atomic<bool> m_lock { false };
};

}