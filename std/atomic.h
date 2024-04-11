#pragma once

#include <std/enums.h>

namespace std {

enum class MemoryOrder {
    Relaxed  = __ATOMIC_RELAXED,
    Consume  = __ATOMIC_CONSUME,
    Acquire  = __ATOMIC_ACQUIRE,
    Release  = __ATOMIC_RELEASE,
    AcqRel   = __ATOMIC_ACQ_REL,
    SeqCst   = __ATOMIC_SEQ_CST
};

template<typename T>
class Atomic {
public:
    Atomic() = default;
    Atomic(T value) : m_value(value) {}

    Atomic(const Atomic&) = delete;
    Atomic& operator=(const Atomic&) = delete;

    T exchange(T value, MemoryOrder order = MemoryOrder::SeqCst) volatile {
        return __atomic_exchange_n(&m_value, value, to_underlying(order));
    }

    bool compare_exchange_strong(T& expected, T desired, MemoryOrder order = MemoryOrder::SeqCst) volatile {
        if (order == MemoryOrder::AcqRel || order == MemoryOrder::Release) {
            return __atomic_compare_exchange_n(
                &m_value, &expected, desired, false, to_underlying(MemoryOrder::Release), to_underlying(MemoryOrder::Acquire)
            );
        }

        return __atomic_compare_exchange_n(&m_value, &expected, desired, false, to_underlying(order), to_underlying(order));
    }

    T load(MemoryOrder order = MemoryOrder::SeqCst) const volatile {
        return __atomic_load_n(&m_value, to_underlying(order));
    }

    void store(T value, MemoryOrder order = MemoryOrder::SeqCst) volatile {
        __atomic_store_n(&m_value, value, to_underlying(order));
    }

    T fetch_add(T value, MemoryOrder order = MemoryOrder::SeqCst) volatile {
        return __atomic_fetch_add(&m_value, value, to_underlying(order));
    }


private:
    T m_value = 0;
};

}