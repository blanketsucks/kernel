#pragma once

#include <kernel/common.h>

#include <std/string_view.h>
#include <std/format.h>

#define PANIC(message) kernel::panic(message, __FILE__, __LINE__)

#define ASSERT(condition, ...)                                       \
    do {                                                             \
        if (!(condition)) { PANIC(std::format(__VA_ARGS__)); }       \
    } while (0)                                     

namespace kernel {

struct StackFrame {
    StackFrame* bp;
    FlatPtr ip;
};

StackFrame* get_kernel_stack_frame();

void print_stack_trace(StackFrame*);
void print_stack_trace();

[[noreturn]] void panic(std::StringView message);
[[noreturn]] void panic();
[[noreturn]] void panic(std::StringView message, const char* file, u32 line);

}