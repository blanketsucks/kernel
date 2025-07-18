#pragma once

#include <std/types.h>
#include <std/string.h>
#include <std/stb_sprintf.h>

namespace std {

struct FormatStyle {
    bool hex = false;
    bool pointer = false;
    bool uppercase = false;

    bool prefix = false;
    bool character = false;

    bool left_pad = false;
    bool right_pad = false;

    u16 width = 0;
    u16 integer_padding = 0;
};

struct FormatBuffer {
    FormatBuffer() : m_position(0) {}

    size_t position() const { return m_position; }
    String value() const { return m_buffer; }

    void append(char ch) { m_buffer.append(ch); }
    
    void append(const char* str, size_t length) {
        if (length == 0) {
            return;
        }

        m_buffer.append(str, length);
    }

    void append(const char* str) { m_buffer.append(str); }
    void append(const String& str) { m_buffer.append(str); }

    void appendf(const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
        va_list args;
        va_start(args, fmt);

        static char buffer[4096];
        memset(buffer, 0, sizeof(buffer));

        stbsp_vsnprintf(buffer, 4096, fmt, args);

        this->append(buffer);
        va_end(args);
    }

private:
    String m_buffer;
    size_t m_position;
};

template<typename T>
struct Formatter {};

template<typename T, bool is_signed> 
void __format_integer(FormatBuffer& buffer, const T& value, const FormatStyle& style) {
    char temp[70];

    if (style.pointer) {
        if (style.prefix) {
            stbsp_sprintf(temp, "0x%p", reinterpret_cast<void*>(value));
        } else {
            stbsp_sprintf(temp, "%p", reinterpret_cast<void*>(value));
        }
    } else if (style.hex) {
        if (style.prefix) {
            if (!style.uppercase) {
                stbsp_sprintf(temp, "%#x", value);
            } else {
                stbsp_sprintf(temp, "%#X", value);
            }
        } else {
            if (!style.uppercase) {
                stbsp_sprintf(temp, "%x", value);
            } else {
                stbsp_sprintf(temp, "%X", value);
            }
        }
    } else if (style.character) {
        stbsp_sprintf(temp, "%c", static_cast<char>(value));
    } else {
        if constexpr (is_signed) {
            stbsp_sprintf(temp, "%d", value);
        } else {
            stbsp_sprintf(temp, "%u", value);
        }
    }

    if (style.integer_padding > 0) {
        size_t length = strlen(temp);
        size_t padding = style.integer_padding - length;

        for (size_t i = 0; i < padding; i++) {
            buffer.append('0');
        }
    }

    buffer.append(temp);
}

template<typename T, bool is_signed>
void __format_64_integer(FormatBuffer& buffer, const T& value, const FormatStyle& style) {
    char temp[70];

    if (style.pointer) {
        if (style.prefix) {
            stbsp_sprintf(temp, "0x%p", reinterpret_cast<void*>(value));
        } else {
            stbsp_sprintf(temp, "%p", reinterpret_cast<void*>(value));
        }
    } else if (style.hex) {
        if (style.prefix) {
            if (!style.uppercase) {
                stbsp_sprintf(temp, "%#lx", value);
            } else {
                stbsp_sprintf(temp, "%#lX", value);
            }
        } else {
            if (!style.uppercase) {
                stbsp_sprintf(temp, "%lx", value);
            } else {
                stbsp_sprintf(temp, "%lX", value);
            }
        }
    } else if (style.character) {
        stbsp_sprintf(temp, "%c", static_cast<char>(value));
    } else {
        if constexpr (is_signed) {
            stbsp_sprintf(temp, "%ld", value);
        } else {
            stbsp_sprintf(temp, "%lu", value);
        }
    }

    if (style.integer_padding > 0) {
        size_t length = strlen(temp);
        size_t padding = style.integer_padding - length;

        for (size_t i = 0; i < padding; i++) {
            buffer.append('0');
        }
    }

    buffer.append(temp);
}

#define _INTEGER_FORMATTER(Type, signed)                                                            \
    template<> struct Formatter<Type> {                                                             \
        static void format(FormatBuffer& buffer, const Type& value, const FormatStyle& style) {     \
            __format_integer<Type, signed>(buffer, value, style);                                   \
        }                                                                                           \
    };

_INTEGER_FORMATTER(i8, true)
_INTEGER_FORMATTER(i16, true)
_INTEGER_FORMATTER(i32, true)
_INTEGER_FORMATTER(u8, false)
_INTEGER_FORMATTER(u16, false)
_INTEGER_FORMATTER(u32, false)

#ifdef __x86__
    _INTEGER_FORMATTER(unsigned long, false)
    _INTEGER_FORMATTER(long, true)
#endif

#undef _INTEGER_FORMATTER

template<> struct Formatter<u64> {
    static void format(FormatBuffer& buffer, const u64& value, const FormatStyle& style) {
        __format_64_integer<u64, false>(buffer, value, style);
    }
};

template<> struct Formatter<i64> {
    static void format(FormatBuffer& buffer, const i64& value, const FormatStyle& style) {
        __format_64_integer<i64, true>(buffer, value, style);
    }
};

template<> struct Formatter<String> {
    static void format(FormatBuffer& buffer, const String& value, const FormatStyle&) {
        buffer.append(value);
    }
};

template<> struct Formatter<StringView> {
    static void format(FormatBuffer& buffer, const StringView& value, const FormatStyle&) {
        buffer.append(value.data(), value.size());
    }
};

template<> struct Formatter<char> {
    static void format(FormatBuffer& buffer, const char& value, const FormatStyle&) {
        buffer.append(value);
    }
};

template<> struct Formatter<const char*> {
    static void format(FormatBuffer& buffer, const char* value, const FormatStyle&) {
        if (value == nullptr) {
            buffer.append("(null)");
            return;
        }

        buffer.append(value);
    }
};

template<> struct Formatter<char*> {
    static void format(FormatBuffer& buffer, char* value, const FormatStyle&) {
        buffer.append(value);
    }
};

template<> struct Formatter<bool> {
    static void format(FormatBuffer& buffer, const bool& value, const FormatStyle&) {
        buffer.append(value ? "true" : "false");
    }
};

template<typename T> struct Formatter<T*> {
    static void format(FormatBuffer& buffer, const T* value, const FormatStyle& style) {
        char temp[66];
        if (style.prefix) {
            stbsp_sprintf(temp, "0x%p", value);
        } else {
            stbsp_sprintf(temp, "%p", value);
        }

        buffer.append(temp);
    }
};

#ifndef __KERNEL__

template<> struct Formatter<float> {
    static void format(FormatBuffer& buffer, const float& value, const FormatStyle&) {
        char temp[64];
        stbsp_sprintf(temp, "%f", value);
        buffer.append(temp);
    }
};

template<> struct Formatter<double> {
    static void format(FormatBuffer& buffer, const double& value, const FormatStyle&) {
        char temp[64];
        stbsp_sprintf(temp, "%f", value);
        buffer.append(temp);
    }
};

#endif

struct has_formatter_impl {
    template<typename T>
    static std::true_type test(int, decltype(&Formatter<T>::format));

    template<typename T>
    static std::false_type test(...);
};

template<typename T>
struct has_formatter {
    static constexpr bool value = decltype(has_formatter_impl::test<T>(0, nullptr))::value;
};

template<typename T>
inline constexpr bool has_formatter_v = has_formatter<T>::value;

struct FormatParameter {
    void const* value;
    void (*format)(FormatBuffer&, const void*, const FormatStyle&);
};

template<typename T>
void __format_value(FormatBuffer& buffer, void const* value, const FormatStyle& style) {
    Formatter<T>::format(buffer, *reinterpret_cast<T const*>(value), style);
}

class FormatParameters {
public:
    FormatParameters(u32 size) : size(size), index(0) { }

    u32 size;
    u32 index;

    FormatParameter parameters[0];
};

template<typename... Args>
class VariadicFormatParameters : public FormatParameters {
public:
    static constexpr size_t size = sizeof...(Args);

    VariadicFormatParameters(Args const&... args) : FormatParameters(size), m_parameter_storage { { &args, __format_value<Args> }... } { }

private:
    FormatParameter m_parameter_storage[size];
};

void _format_impl(FormatBuffer&, const char* fmt, FormatParameters&);
void _dbg_impl(const char* fmt, FormatParameters&, bool newline);

template<typename... Args> requires(std::conjunction<has_formatter<Args>...>::value)
String format(const char* fmt, Args const&... args) {
    FormatBuffer buffer;
    VariadicFormatParameters<Args...> params(args...);

    _format_impl(buffer, fmt, params);
    return buffer.value();
}

template<typename... Args> requires(std::conjunction<has_formatter<Args>...>::value)
void dbg(const char* fmt, Args const&... args) {
    VariadicFormatParameters<Args...> params(args...);
    _dbg_impl(fmt, params, false);
}

template<typename... Args> requires(std::conjunction<has_formatter<Args>...>::value)
void dbgln(const char* fmt, Args const&... args) {
    VariadicFormatParameters<Args...> params(args...);
    _dbg_impl(fmt, params, true);
}

void dbgln(StringView);
void dbgln();

}

using std::dbg;
using std::dbgln;