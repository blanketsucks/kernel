#pragma once

#include <std/kmalloc.h>

namespace std {

template<typename T>
class HeapArray {
public:
    explicit HeapArray(size_t count) { allocate(count); }

    ~HeapArray() { release(); }

    HeapArray(const HeapArray&) = delete;
    HeapArray& operator=(const HeapArray&) = delete;

    HeapArray(HeapArray&& other) : m_data(other.m_data), m_size(other.m_size) {
        other.m_data = nullptr;
        other.m_size = 0;
    }

    HeapArray& operator=(HeapArray&& other) {
        if (this != &other) {
            release();

            m_data = other.m_data;
            m_size = other.m_size;

            other.m_data = nullptr;
            other.m_size = 0;
        }

        return *this;
    }

    T& operator[](size_t i) { return m_data[i]; }
    T const& operator[](size_t i) const { return m_data[i]; }

    T* data() { return m_data; }
    T const* data() const { return m_data; }

    size_t size() const { return m_size; }

    T* begin() { return m_data; }
    T* end() { return m_data + m_size; }

private:
    void allocate(size_t count) {
        if (!count) {
            return;
        }

        m_data = new T[count];
        m_size = count;

        for (size_t i = 0; i < count; i++) {
            new (&m_data[i]) T();
        }
    }

    void release() {
        if (!m_data) {
            return;
        }

        for (size_t i = m_size; i > 0; i--) {
            m_data[i - 1].~T();
        }

        delete m_data;

        m_data = nullptr;
        m_size = 0;
    }

    T* m_data = nullptr;
    size_t m_size = 0;
};

}
