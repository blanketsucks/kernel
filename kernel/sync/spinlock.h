#pragma once

#include <kernel/common.h>

#include <stdatomic.h>

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
    atomic_flag lock { static_cast<atomic_bool>(0) };
};

}