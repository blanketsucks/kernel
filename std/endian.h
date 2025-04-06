#pragma once

#include <std/type_traits.h>
#include <std/format.h>

namespace std {

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    static constexpr bool is_big_endian = false;
#else
    static constexpr bool is_big_endian = true;
#endif

template<typename T> requires(std::is_integral_v<T>)
constexpr T byte_swap(T value) {
    if constexpr (sizeof(T) == 1) {
        return value;
    } else if constexpr (sizeof(T) == 2) {
        return __builtin_bswap16(value);
    } else if constexpr (sizeof(T) == 4) {
        return __builtin_bswap32(value);
    } else if constexpr (sizeof(T) == 8) {
        return __builtin_bswap64(value);
    }
}

template<typename T> requires(std::is_integral_v<T>)
constexpr T to_big_endian(T value) {
    if constexpr (is_big_endian) {
        return value;
    } else {
        return byte_swap(value);
    }
}

template<typename T> requires(std::is_integral_v<T>)
constexpr T to_little_endian(T value) {
    if constexpr (is_big_endian) {
        return byte_swap(value);
    } else {
        return value;
    }
}

template<typename T> requires(std::is_integral_v<T>)
constexpr T from_big_endian(T value) {
    return to_big_endian(value);
}

template<typename T> requires(std::is_integral_v<T>)
constexpr T from_little_endian(T value) {
    return to_little_endian(value);
}

template<typename T> requires(std::is_integral_v<T>)
class LittleEndian {
public:
    constexpr LittleEndian() = default;

    constexpr LittleEndian(T value) : m_value(to_little_endian(value)) {}

    constexpr LittleEndian& operator=(T value) {
        m_value = to_little_endian(value);
        return *this;
    }

    constexpr operator T() const { return to_little_endian(m_value); }
    constexpr T value() const { return to_little_endian(m_value); }

private:
    T m_value;
};

template<typename T> requires(std::is_integral_v<T>)
class BigEndian {
public:
    constexpr BigEndian() = default;

    constexpr BigEndian(T value) : m_value(to_big_endian(value)) {}

    constexpr BigEndian& operator=(T value) {
        m_value = to_big_endian(value);
        return *this;
    }

    constexpr operator T() const { return to_big_endian(m_value); }
    constexpr T value() const { return to_big_endian(m_value); }

private:
    T m_value;
};

template<typename T> requires(std::is_integral_v<T>)
using NetworkOrder = BigEndian<T>;

}

template<typename T> requires(std::is_integral_v<T>)
struct std::Formatter<std::BigEndian<T>> {
    static void format(FormatBuffer& buffer, const std::BigEndian<T>& value, const FormatStyle& style) {
        std::Formatter<T>::format(buffer, value.value(), style);
    }
};

template<typename T> requires(std::is_integral_v<T>)
struct std::Formatter<std::LittleEndian<T>> {
    static void format(FormatBuffer& buffer, const std::LittleEndian<T>& value, const FormatStyle& style) {
        std::Formatter<T>::format(buffer, value.value(), style);
    }
};