#pragma once

#include <std/types.h>
#include <std/result.h>
#include <std/memory.h>

namespace std {

class RingBuffer {
public:
    static OwnPtr<RingBuffer> create(size_t capacity) {
        auto* buffer = new u8[capacity];
        return OwnPtr(new RingBuffer(buffer, capacity));
    }

    size_t capacity() const { return m_capacity; }
    size_t size() const { return m_size; }

    bool full() const { return m_size >= m_capacity; }
    bool empty() const { return m_size == 0; }

    ErrorOr<size_t> read(u8* buffer, size_t count);
    ErrorOr<size_t> write(u8 const* buffer, size_t count);

private:
    RingBuffer(u8* buffer, size_t capacity) : m_buffer(buffer), m_capacity(capacity) {}

    size_t m_tail = 0;
    size_t m_head = 0;

    u8* m_buffer = nullptr;

    size_t m_capacity = 0;
    size_t m_size     = 0;
};

}