#pragma once

#include <kernel/common.h>
#include <kernel/posix/sys/types.h>

#include <std/atomic.h>

namespace kernel {

class Mutex {
public:
    Mutex() = default;

    NO_COPY(Mutex)
    NO_MOVE(Mutex)

    void lock();
    void unlock();

    pid_t owner() const { return m_owner.load(std::MemoryOrder::Relaxed); }

    bool is_locked();

private:
    std::Atomic<pid_t> m_owner { -1 };
    std::Atomic<size_t> m_recursion_count { 0 };
};

}