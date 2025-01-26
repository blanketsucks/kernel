#include <kernel/panic.h>
#include <kernel/symbols.h>
#include <kernel/serial.h>
#include <kernel/vga.h>
#include <std/format.h>

namespace kernel {

StackFrame* get_stack_frame() {
    StackFrame* frame = nullptr;
#if __x86_64__
    asm volatile("mov %%rbp, %0" : "=r"(frame));
#else
    asm volatile("mov %%ebp, %0" : "=r"(frame));
#endif

    return frame;
}

void print_stack_trace() {
    if (!has_loaded_symbols()) {
        return;
    }

    StackFrame* frame = kernel::get_stack_frame();
    while (frame) {
        if (!MM->is_mapped(reinterpret_cast<void*>(frame->eip))) {
            dbgln(" - ??? at {:#x}", frame->eip);
            break;
        }

        Symbol* symbol = resolve_symbol(frame->eip);
        if (!symbol) {
            dbgln(" - ??? at {:#x}", frame->eip);
        } else {
            dbgln(" - {} at {:#x}", StringView { symbol->name }, frame->eip);
        }

        frame = frame->ebp;
    }
}

static void output_to_serial(const char* message, const char* file, u32 line) {
    if (file && line) {
        serial::printf("PANIC(%s:%u): %s\n", file, line, message);
    } else {
        serial::printf("PANIC: %s\n", message);
    }
}

static void output_to_vga(const char* message, const char* file, u32 line) {
    if (file && line) {
        vga::printf("PANIC(%s:%u): %s\n", file, line, message);
    } else {
        vga::printf("PANIC: %s\n", message);
    }
}

[[noreturn]] void panic(const char* message, bool vga) {
    panic(message, nullptr, 0, vga);
}

[[noreturn]] void panic(const char* message, const char* file, u32 line, bool vga) {
    if (!serial::is_initialized()) serial::init();

    output_to_serial(message, file, line);
    if (vga) output_to_vga(message, file, line);

    print_stack_trace();

    asm volatile("cli");
    asm volatile("hlt");

    __builtin_unreachable();
}


}