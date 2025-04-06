#pragma once

#include <kernel/common.h>

namespace std {

template<typename T, size_t N>
struct Array {
    using Iterator = T*;
    using ConstIterator = const T*;

    constexpr Array() = default;

    constexpr Iterator begin() { return m_data; }
    constexpr ConstIterator begin() const { return m_data; }

    constexpr Iterator end() { return m_data + N; }
    constexpr ConstIterator end() const  { return m_data + N; }

    constexpr size_t size() const { return N; }
    constexpr bool empty() const { return N == 0; }

    constexpr T& operator[](size_t index) { return m_data[index]; }
    constexpr const T& operator[](size_t index) const { return m_data[index]; }

    constexpr T* data() { return m_data; }
    constexpr const T* data() const { return m_data; }

    constexpr T& first() { return m_data[0]; }
    constexpr const T& first() const { return m_data[0]; }

    constexpr T& last() { return m_data[N - 1]; }
    constexpr const T& last() const { return m_data[N - 1]; }

    constexpr void fill(const T& value) {
        for (size_t i = 0; i < N; i++) {
            m_data[i] = value;
        }
    }

private:
    T m_data[N];
};

}

using std::Array;