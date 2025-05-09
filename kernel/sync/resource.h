#pragma once

#include <kernel/sync/spinlock.h>

#include <std/utility.h>

namespace kernel {

template<typename T, typename Lock>
class LockedResource {
public:
    template<typename... Args>
    LockedResource(Args&&... args) : m_value(std::forward<Args>(args)...) {}

    LockedResource(LockedResource&& other) = delete;
    LockedResource(const LockedResource& other) = delete;

    LockedResource& operator=(LockedResource&& other) = delete;
    LockedResource& operator=(const LockedResource& other) = delete;

    template<typename F>
    auto with(F&& func) {
        ScopedSpinLock lock(m_lock);
        return func(m_value);
    }

    template<typename F>
    auto with(F&& func) const {
        ScopedSpinLock lock(m_lock);
        return func(m_value);
    }

private:
    T m_value;
    mutable Lock m_lock;
};

template<typename T>
using SpinLockResource = LockedResource<T, SpinLock>;

}