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
};

struct FormatBuffer {
    FormatBuffer() : m_position(0) {}

    size_t position() const { return m_position; }
    String value() const { return m_buffer; }

    void append(char ch) { m_buffer.append(ch); }
    void append(const char* str, size_t length) { m_buffer.append(str, length); }
    void append(const char* str) { m_buffer.append(str); }
    void append(const String& str) { m_buffer.append(str); }

    void appendf(const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
        va_list args;
        va_start(args, fmt);

        char buffer[4096];
        stbsp_vsnprintf(buffer, 4096, fmt, args);

        this->append(buffer);
        va_end(args);
    }

private:
    String m_buffer;
    size_t m_position;
};

template<typename T>
struct Formatter {
    static void format(FormatBuffer&, const T& value, const FormatStyle&);
};

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
                stbsp_sprintf(temp, "%#llx", value);
            } else {
                stbsp_sprintf(temp, "%#llX", value);
            }
        } else {
            if (!style.uppercase) {
                stbsp_sprintf(temp, "%llx", value);
            } else {
                stbsp_sprintf(temp, "%llX", value);
            }
        }
    } else if (style.character) {
        stbsp_sprintf(temp, "%c", static_cast<char>(value));
    } else {
        if constexpr (is_signed) {
            stbsp_sprintf(temp, "%lld", value);
        } else {
            stbsp_sprintf(temp, "%llu", value);
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

_INTEGER_FORMATTER(unsigned long, false)
_INTEGER_FORMATTER(long, true)

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

template<> struct Formatter<f32> {
    static void format(FormatBuffer& buffer, const f32& value, const FormatStyle&) {
        char temp[64];
        stbsp_sprintf(temp, "%f", value);
        buffer.append(temp);
    }
};

template<> struct Formatter<f64> {
    static void format(FormatBuffer& buffer, const f64& value, const FormatStyle&) {
        char temp[64];
        stbsp_sprintf(temp, "%f", value);
        buffer.append(temp);
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

template<typename... Args>
String format(const char* fmt, Args const&... args) {
    FormatBuffer buffer;
    VariadicFormatParameters<Args...> params(args...);

    _format_impl(buffer, fmt, params);
    return buffer.value();
}

template<typename... Args>
void dbg(const char* fmt, Args const&... args) {
    VariadicFormatParameters<Args...> params(args...);
    _dbg_impl(fmt, params, false);
}

template<typename... Args>
void dbgln(const char* fmt, Args const&... args) {
    VariadicFormatParameters<Args...> params(args...);
    _dbg_impl(fmt, params, true);
}

}

using std::dbg;
using std::dbgln;