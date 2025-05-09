#pragma once

#include <std/types.h>

namespace std {

class BytesBuffer {
public:
    BytesBuffer() = default;
    BytesBuffer(void* buffer, size_t size) : m_buffer(reinterpret_cast<u8*>(buffer)), m_size(size) {}

    size_t size() const { return m_size; }
    size_t offset() const { return m_offset; }

    void reset() { m_offset = 0; }

    void advance(size_t offset) {
        if (m_offset + offset > m_size) {
            return;
        }

        m_offset += offset;
    }

    template<typename T>
    void write(const T& value) {
        if (m_offset + sizeof(T) > m_size) {
            return;
        }

        memcpy(m_buffer + m_offset, &value, sizeof(T));
        m_offset += sizeof(T);
    }

    template<typename T>
    T* read() {
        if (m_offset + sizeof(T) > m_size) {
            return nullptr;
        }

        T* value = reinterpret_cast<T*>(m_buffer + m_offset);
        m_offset += sizeof(T);

        return value;
    }

private:
    u8* m_buffer = nullptr;

    size_t m_size = 0;
    size_t m_offset = 0;
};

}