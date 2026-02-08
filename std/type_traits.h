#pragma once

namespace std {

template<class T> struct remove_reference { using Type = T; };
template<class T> struct remove_reference<T&> { using Type = T; };
template<class T> struct remove_reference<T&&> { using Type = T; };

template<typename T> using remove_reference_t = typename remove_reference<T>::Type;

template<typename T> inline constexpr bool is_enum = __is_enum(T);

template<typename T> requires(is_enum<T>)
using underlying_type = __underlying_type(T);

template<typename T, T v>
struct integral_constant {
    using value_type = T;
    using type = integral_constant;

    static constexpr T value = v;
};

using true_type = integral_constant<bool, true>;
using false_type = integral_constant<bool, false>;

template<bool B, typename T, typename F>
struct conditional { using type = T; };

template<typename T, typename F>
struct conditional<false, T, F> { using type = F; };

template<bool B, typename T, typename F>
using conditional_t = typename conditional<B, T, F>::type;

template<typename...>
struct conjunction : true_type {};

template<typename T, typename... Ts>
struct conjunction<T, Ts...> : conditional_t<bool(T::value), conjunction<Ts...>, T> {};

template<typename T, typename U>
struct is_same {
    static constexpr bool value = false;
};

template<typename T>
struct is_same<T, T> {
    static constexpr bool value = true;
};

template<typename T, typename U>
inline constexpr bool is_same_v = is_same<T, U>::value;

template<bool B, typename T = void>
struct enable_if {};
 
template<class T>
struct enable_if<true, T> { using type = T; };

template<bool B, typename T = void>
using enable_if_t = typename enable_if<B, T>::type;

template<typename T>
struct type_identity {
    using type = T;
};

namespace detail {

template<typename T>
auto try_add_lvalue_reference(int) -> type_identity<T&>;
template<typename T>
auto try_add_lvalue_reference(...) -> type_identity<T>;

template<typename T>
auto try_add_rvalue_reference(int) -> type_identity<T&&>;
template<typename T>
auto try_add_rvalue_reference(...) -> type_identity<T>;

template<typename T>
true_type test_ptr_conv(const volatile T*);

template<typename T>
false_type test_ptr_conv(const volatile void*);

template<typename B, typename D>
auto test_is_base_of(int) -> decltype(test_ptr_conv<B>(static_cast<D*>(nullptr)));
template<typename, typename>
auto test_is_base_of(...) -> true_type;

}

template<typename T>
struct add_lvalue_reference : decltype(detail::try_add_lvalue_reference<T>(0)) {};

template<typename T>
struct add_rvalue_reference : decltype(detail::try_add_rvalue_reference<T>(0)) {};

template<typename T>
using add_lvalue_reference_t = typename add_lvalue_reference<T>::type;

template<typename T>
using add_rvalue_reference_t = typename add_rvalue_reference<T>::type;

template<typename T>
add_rvalue_reference_t<T> declval() noexcept;

template<typename T>
struct remove_cv { using type = T; };

template<typename T>
struct remove_cv<const T> { using type = T; };

template<typename T>
struct remove_cv<volatile T> { using type = T; };

template<typename T>
struct remove_cv<const volatile T> { using type = T; };

template<typename T>
struct decay {
private:
    using U = remove_reference_t<T>;
public:
    using type = remove_cv<U>; // FIXME: Properly implement decay
};

template<typename T>
using decay_t = typename decay<T>::type;

template<typename B, typename D>
struct is_base_of : decltype(detail::test_is_base_of<B, D>(0)) {};

template<typename B, typename D>
inline constexpr bool is_base_of_v = is_base_of<B, D>::value;

#ifdef __clang__

template<typename T> inline constexpr bool is_integral_v = __is_integral(T);
template<typename T> inline constexpr bool is_signed_v = __is_signed(T);
template<typename T> inline constexpr bool is_unsigned_v = __is_unsigned(T);

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

template<typename T, typename U>
concept same_as = is_same_v<T, U> && is_same_v<U, T>;

}