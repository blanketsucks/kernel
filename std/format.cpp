#include <std/format.h>
#include <std/cstring.h>

#ifdef __KERNEL__
    #include <kernel/serial.h>
    #include <kernel/process/scheduler.h>
    #include <kernel/process/threads.h>
    #include <kernel/process/process.h>
#else
    #include <unistd.h>
    #include <stdio.h>
#endif

namespace std {

struct StyleResult {
    FormatStyle style;
    const char* fmt;
};  

StyleResult parse_format_style(const char* fmt) {
    FormatStyle style = {};
    if (*fmt == '}') {
        return { style, fmt };
    }

    if (*fmt == '-') {
        style.right_pad = true;
        fmt++;
    }

    if (isdigit(*fmt)) {
        style.left_pad = !style.right_pad;

        u16 width = 0;
        while (isdigit(*fmt)) {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        style.width = width;
    }

    if (*fmt == ':') {
        fmt++;
    } else {
        return { style, fmt };
    }


    if (*fmt == '#') {
        style.prefix = true;
        fmt++;
    }

    switch (*fmt) {
        case 'x': style.hex = true; break;
        case 'X': style.hex = true; style.uppercase = true; break;
        case 'p': style.pointer = true; break;
        case 'c': style.character = true; break;
    }

    while (*fmt && *fmt != '}') {
        fmt++;
    }

    return { style, fmt };
}

void _format_impl(FormatBuffer& buffer, const char* fmt, FormatParameters& params) {
    while (*fmt) {
        if (*fmt != '{') {
            buffer.append(*fmt);
            fmt++;

            continue;
        }

        auto [style, result] = parse_format_style(++fmt);
        fmt = result;

        if (*fmt != '}') {
            fmt++;
            continue;
        }

        // TODO: Let the user choose the padding character.
        if (style.left_pad) {
            for (u32 i = 0; i < style.width; i++) buffer.append(' ');
        }

        fmt++;
        auto& parameter = params.parameters[params.index];

        parameter.format(buffer, parameter.value, style);
        params.index++;

        if (style.right_pad) {
            for (u32 i = 0; i < style.width; i++) buffer.append(' ');
        }
    }
}

void _dbg_impl(const char* fmt, FormatParameters& params, bool newline) {
    FormatBuffer buffer;

#ifdef __KERNEL__
    auto* thread = kernel::Scheduler::current_thread();
    if (!thread) {
        buffer.append("[Kernel]: ");
    } else {
        auto* process = thread->process();
        auto& name = process->name();

        buffer.appendf("[%.*s (PID %u)]: ", name.size(), name.data(), process->id());
    }
#endif

    if (params.size == 0) {
        buffer.append(fmt);
    } else {
        _format_impl(buffer, fmt, params);
    }

    if (newline) {
        buffer.append('\n');
    }

    String value = buffer.value();
    
#ifdef __KERNEL__
    kernel::serial::write(value.data(), value.size());
#else
    write(1, value.data(), value.size());
#endif
}

void dbgln() {
    FormatParameters params(0);
    _dbg_impl("", params, true);
}

}