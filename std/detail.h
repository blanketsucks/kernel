#pragma once

namespace std::detail {

template<class T> struct remove_reference { using Type = T; };
template<class T> struct remove_reference<T&> { using Type = T; };
template<class T> struct remove_reference<T&&> { using Type = T; };

template<typename T> using remove_reference_t = typename remove_reference<T>::Type;

template<typename T> inline constexpr bool is_enum = __is_enum(T);

template<typename T> requires(is_enum<T>)
using underlying_type = __underlying_type(T);

#ifdef __clang__

template<typename T> inline constexpr bool is_integral_v = __is_integral(T);
template<typename T> inline constexpr bool is_signed_v = __is_signed(T);

#else

template<typename T> struct is_integral { static constexpr bool value = false; };

template<> struct is_integral<bool> { static constexpr bool value = true; };

template<> struct is_integral<char> { static constexpr bool value = true; };
template<> struct is_integral<signed char> { static constexpr bool value = true; };
template<> struct is_integral<unsigned char> { static constexpr bool value = true; };

template<> struct is_integral<short> { static constexpr bool value = true; };
template<> struct is_integral<unsigned short> { static constexpr bool value = true; };

template<> struct is_integral<int> { static constexpr bool value = true; };
template<> struct is_integral<unsigned int> { static constexpr bool value = true; };

template<> struct is_integral<long> { static constexpr bool value = true; };
template<> struct is_integral<unsigned long> { static constexpr bool value = true; };

template<> struct is_integral<long long> { static constexpr bool value = true; };
template<> struct is_integral<unsigned long long> { static constexpr bool value = true; };

template<typename T> inline constexpr bool is_integral_v = is_integral<T>::value;

#endif

}