#include <kernel/panic.h>
#include <kernel/symbols.h>
#include <kernel/serial.h>
#include <kernel/memory/manager.h>


#include <std/format.h>

namespace kernel {

StackFrame* get_kernel_stack_frame() {
    StackFrame* frame = nullptr;
#if __x86_64__
    asm volatile("mov %%rbp, %0" : "=r"(frame));
#else
    asm volatile("mov %%ebp, %0" : "=r"(frame));
#endif

    return frame;
}

void print_stack_trace() {
    print_stack_trace(get_kernel_stack_frame());
}

void print_stack_trace(StackFrame* frame) {
    while (frame) {
        if (!has_loaded_symbols()) {
            dbgln(" - ??? at {:#x}", frame->ip);

            frame = frame->bp;
            continue;
        }

        if (!MM->is_mapped(reinterpret_cast<void*>(frame->ip))) {
            dbgln(" - ??? at {:#x}", frame->ip);
            break;
        }

        Symbol* symbol = resolve_symbol(frame->ip);
        if (!symbol) {
            dbgln(" - ??? at {:#x}", frame->ip);
        } else {
            dbgln(" - {} at {:#x}", StringView { symbol->name }, frame->ip);
        }

        frame = frame->bp;
    }
}

[[noreturn]] void panic(std::StringView message) {
    panic(message, nullptr, 0);
}

[[noreturn]] void panic(std::StringView message, const char* file, u32 line) {
    if (file) {
        dbgln("PANIC({}:{}) {}", StringView { file }, line, message);
    } else {
        dbgln("PANIC: {}", message);
    }

    print_stack_trace();

    asm volatile("cli");
    asm volatile("hlt");

    __builtin_unreachable();
}


}