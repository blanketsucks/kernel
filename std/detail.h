#pragma once

namespace std::detail {

template<class T> struct remove_reference { using Type = T; };
template<class T> struct remove_reference<T&> { using Type = T; };
template<class T> struct remove_reference<T&&> { using Type = T; };

template<typename T> using remove_reference_t = typename remove_reference<T>::Type;

template<typename T> inline constexpr bool is_enum = __is_enum(T);

template<typename T> inline constexpr bool is_integral = __is_integral(T);
template<typename T> inline constexpr bool is_signed = __is_signed(T);

template<typename T> requires(is_enum<T>)
using underlying_type = __underlying_type(T);

}