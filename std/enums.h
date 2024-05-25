#pragma once

#include <std/detail.h>

#define MAKE_ENUM_BITWISE_OPS(Enum)                                                     \
    constexpr Enum operator|(Enum lhs, Enum rhs) {                                      \
        return static_cast<Enum>(to_underlying(lhs) | to_underlying(rhs));              \
    }                                                                                   \
                                                                                        \
    constexpr Enum operator&(Enum lhs, Enum rhs) {                                      \
        return static_cast<Enum>(to_underlying(lhs) & to_underlying(rhs));              \
    }                                                                                   \
                                                                                        \
    constexpr Enum operator^(Enum lhs, Enum rhs) {                                      \
        return static_cast<Enum>(to_underlying(lhs) ^ to_underlying(rhs));              \
    }                                                                                   \
                                                                                        \
    constexpr Enum operator~(Enum e) {                                                  \
        return static_cast<Enum>(~to_underlying(e));                                    \
    }                                                                                   \
                                                                                        \
    constexpr Enum& operator|=(Enum& lhs, Enum rhs) {                                   \
        lhs = static_cast<Enum>(to_underlying(lhs) | to_underlying(rhs));               \
        return lhs;                                                                     \
    }                                                                                   \
                                                                                        \
    constexpr Enum& operator&=(Enum& lhs, Enum rhs) {                                   \
        lhs = static_cast<Enum>(to_underlying(lhs) & to_underlying(rhs));               \
        return lhs;                                                                     \
    }                                                                                   \
                                                                                        \
    constexpr Enum& operator^=(Enum& lhs, Enum rhs) {                                   \
        lhs = static_cast<Enum>(to_underlying(lhs) ^ to_underlying(rhs));               \
        return lhs;                                                                     \
    }                                                                                   \


namespace std {

template<typename T> inline constexpr bool is_enum = std::detail::is_enum<T>;

template<typename T> requires(is_enum<T>)
using underlying_type = std::detail::underlying_type<T>;

template<typename T> requires(is_enum<T>)
constexpr auto to_underlying(T value) {
    return static_cast<underlying_type<T>>(value);
}

template<typename T> requires(is_enum<T>)
constexpr bool has_flag(T value, T mask) {
    return to_underlying(value & mask) == to_underlying(mask);
}

template<typename T> requires(is_enum<T>)
constexpr bool has_any_flag(T value, T mask) {
    return to_underlying(value & mask) != 0;
}

}

using std::to_underlying;
using std::has_flag;
using std::has_any_flag;