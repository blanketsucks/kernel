#pragma once

#include <std/format.h>

#define ASSERT(condition)                                               \
    do {                                                                \
        if (!(condition)) [[unlikely]] {                                \
            ::std::assert_failed(__FILE__, __LINE__, #condition, {});   \
        }                                                               \
    } while (0)

#define ASSERT_MSG(condition, ...)                                                              \
    do {                                                                                        \
        if (!(condition)) [[unlikely]] {                                                        \
            ::std::assert_failed(__FILE__, __LINE__, #condition, ::std::format(__VA_ARGS__));   \
        }                                                                                       \
    } while (0)

namespace std {

[[noreturn]]
void assert_failed(StringView file, int line, StringView condition, StringView message);

}
