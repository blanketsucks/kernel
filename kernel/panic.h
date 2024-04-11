#pragma once

#include <kernel/common.h>

#define PANIC(message) kernel::panic(message, __FILE__, __LINE__, false)

#define ASSERT(condition, message)                  \
    do {                                            \
        if (!(condition)) { PANIC(message); }       \
    } while (0)                                     

namespace kernel {

struct StackFrame {
    StackFrame* ebp;
    u32 eip;
};

StackFrame* get_stack_frame();
void print_stack_trace();

[[noreturn]] void panic(const char* message, bool vga = true);
[[noreturn]] void panic(const char* message, const char* file, u32 line, bool vga = true);

}