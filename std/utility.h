#pragma once

#include <std/type_traits.h>
#include <std/types.h>

namespace std {

using nullptr_t = decltype(nullptr);

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

template<typename T> requires(std::is_integral_v<T>)
constexpr T abs(T value) {
    return value < 0 ? -value : value;
}

template<typename T, typename U> requires(std::is_integral_v<T> && std::is_integral_v<U>)
constexpr T align_up(T value, U alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

template<typename T, typename U> requires(std::is_integral_v<T> && std::is_integral_v<U>)
constexpr T align_down(T value, U alignment) {
    return value & ~(alignment - 1);
}

template<typename T> requires(std::is_integral_v<T>)
constexpr T align_up(T value, T alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

template<typename T> requires(std::is_integral_v<T>)
constexpr T align_down(T value, T alignment) {
    return value & ~(alignment - 1);
}

template<typename T> requires(std::is_integral_v<T>)
constexpr T ceil_div(T value, T divisor) {
    return (value + divisor - 1) / divisor;
}

template<typename T>
constexpr T&& move(T& value) { 
    return static_cast<T&&>(value);
}

template<typename T>
constexpr T&& forward(std::remove_reference_t<T>& value) {
    return static_cast<T&&>(value);
}

template<typename T>
constexpr T&& forward(std::remove_reference_t<T>&& value) {
    return static_cast<T&&>(value);
}

template<typename T>
void swap(T& a, T& b) {
    T temp = move(a);
    
    a = move(b);
    b = move(temp);
}

template<typename Container, typename F>
bool all(Container& container, F&& f) {
    for (auto& item : container) {
        if (!f(item)) {
            return false;
        }
    }

    return true;
}

template<typename Container, typename F>
bool any(Container& container, F&& f) {
    for (auto& item : container) {
        if (f(item)) {
            return true;
        }
    }

    return false;
}

}

using std::IterationAction;
using std::move;