#pragma once

#include <stddef.h>

namespace std {

#if defined(__clang__)

template<typename T>
class initializer_list {
public:
    initializer_list() noexcept : m_first(nullptr), m_last(nullptr) {}

    size_t size() const noexcept { return m_last - m_first; }

    const T* begin() const noexcept { return m_first; }
    const T* end() const noexcept { return m_last; }

private:
    const T* m_first;
    const T* m_last;
};

#elif defined(__GNUC__)

template<typename T>
class initializer_list {
public:
    initializer_list(const T* array, size_t len) : m_array(array), m_len(len) {}
    initializer_list() noexcept : m_array(0), m_len(0) {}

    size_t size() const noexcept { return m_len; }

    const T* begin() const noexcept { return m_array; }
    const T* end() const noexcept { return m_array + m_len; }

private:
    T* m_array;
    size_t m_len;
};

#endif

}