#include <std/ring_buffer.h>

namespace std {

ErrorOr<size_t> RingBuffer::read(u8* buffer, size_t count) {
    if (count == 0) {
        return 0;
    } else if (m_size == 0) {
        return Error(ENODATA);
    }

    size_t n = min(count, m_size);
    if (m_tail + n > m_capacity) {
        size_t end = m_capacity - m_tail;

        memcpy(buffer, m_buffer + m_tail, end);
        memcpy(buffer + end, m_buffer, n - end);
    } else {
        memcpy(buffer, m_buffer + m_tail, n);
    }

    m_tail = (m_tail + n) % m_capacity;
    m_size -= n;

    return n;
}

ErrorOr<size_t> RingBuffer::write(u8 const* buffer, size_t count) {
    if (count == 0) {
        return 0;
    } else if (full()) {
        return Error(ENOBUFS);
    }

    size_t n = min(count, m_capacity - m_size);
    if (m_head + n > m_capacity) {
        size_t end = m_capacity - m_head;

        memcpy(m_buffer + m_head, buffer, end);
        memcpy(m_buffer, buffer + end, n - end);
    } else {
        memcpy(m_buffer + m_head, buffer, n);
    }

    m_head = (m_head + n) % m_capacity;
    m_size += n;

    return n;
}

}