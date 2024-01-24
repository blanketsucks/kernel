#pragma once

#include <std/detail.h>

namespace std {

enum class IterationAction {
    Continue,
    Break
};

template<typename T>
constexpr T min(const T& a, const T& b) {
    return a < b ? a : b;
}

template<typename T>
constexpr T max(const T& a, const T& b) {
    return a > b ? a : b;
}

template<typename T>
constexpr T&& move(T& value) { 
    return static_cast<T&&>(value);
}

template<typename T>
constexpr T&& forward(detail::remove_reference_t<T>& value) {
    return static_cast<T&&>(value);
}

template<typename T>
constexpr T&& forward(detail::remove_reference_t<T>&& value) {
    return static_cast<T&&>(value);
}

}

using std::IterationAction;

using std::min;
using std::max;
using std::move;