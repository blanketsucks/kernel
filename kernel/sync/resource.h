#pragma once

#include <kernel/sync/spinlock.h>
#include <kernel/sync/lock.h>

#include <std/utility.h>

namespace kernel {

template<typename T, Lock Lock>
class LockedResource {
public:
    template<typename... Args>
    LockedResource(Args&&... args) : m_value(std::forward<Args>(args)...) {}

    NO_COPY(LockedResource)
    NO_MOVE(LockedResource)

    template<typename F>
    auto with(F&& func) {
        ScopedLock lock(m_lock);
        return func(m_value);
    }

    template<typename F>
    auto with(F&& func) const {
        ScopedLock lock(m_lock);
        return func(m_value);
    }

private:
    T m_value;
    mutable Lock m_lock;
};

template<typename T>
using SpinLockResource = LockedResource<T, SpinLock>;

}