#pragma once

#include <kernel/common.h>
#include <kernel/posix/sys/types.h>

#include <std/atomic.h>

namespace kernel {

class SpinLock {
public:
    SpinLock() = default;

    NO_COPY(SpinLock)
    NO_MOVE(SpinLock)

    bool is_locked();

    void lock();
    void unlock();

private:
    std::Atomic<u8> m_lock { 0 };
};

}