#pragma once

#include <kernel/common.h>
#include <std/type_traits.h>

namespace kernel {

template<typename T>
concept Lock = requires (T& lock) {
    { lock.lock() } -> std::same_as<void>;
    { lock.unlock() } -> std::same_as<void>;
};

template<Lock Lock>
class ScopedLock {
public:
    explicit ScopedLock(Lock& lock) : m_lock(lock) {
        m_lock.lock();
    }

    ~ScopedLock() {
        m_lock.unlock();
    }

private:
    Lock& m_lock;
};

}