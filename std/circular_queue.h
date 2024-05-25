#pragma once

#include <std/types.h>
#include <std/utility.h>
#include <std/kmalloc.h>

namespace std {

template<typename T, size_t N>
class CircularQueue {
public:
    CircularQueue() = default;
    ~CircularQueue() { clear(); }

    T* data() { return m_data; }
    const T* data() const { return m_data; }

    size_t size() const { return m_size; }

    size_t head() const { return m_head; }
    size_t tail() const { return m_tail; }

    bool empty() const { return m_size == 0; }
    bool full() const { return m_size == N; }

    void clear() {
        for (size_t i = 0; i < m_size; i++) {
            m_data[(m_tail + i) % N].~T();
        }

        m_size = 0;
        m_head = 0;
    }

    void push(const T& value) {
        auto& slot = m_data[(m_head + m_size) % N];
        if (m_size == N) {
            m_tail = (m_tail + 1) % N;
            slot.~T();
        } else {
            m_size++;
        }

        new (&slot) T(move(value));
    }

    T pop() {
        if (m_size == 0) {
            return T();
        }

        auto& slot = m_data[m_tail];
        m_tail = (m_tail + 1) % N;
        m_size--;

        slot.~T();
        return slot;
    }

private:
    T m_data[N];
    size_t m_size;

    size_t m_head = 0;
    size_t m_tail = 0;
    
};

}

using std::CircularQueue;