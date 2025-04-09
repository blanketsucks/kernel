#include <kernel/vga.h>
#include <kernel/arch/io.h>
#include <std/string.h>

#include <stdarg.h>

namespace kernel {

static u32 x = 0;
static u32 y = 0;

void vga::enable_cursor(u8 start, u8 end) {
    io::write<u8>(0x3D4, 0x0A);
    io::write<u8>(0x3D5, (io::read<u8>(0x3D5) & 0xC0) | start);

    io::write<u8>(0x3D4, 0x0B);
    io::write<u8>(0x3D5, (io::read<u8>(0x3E0) & 0xE0) | end);
}

void vga::disable_cursor() {
    io::write<u8>(0x3D4, 0x0A);
    io::write<u8>(0x3D5, 0x20);
}

void vga::set_cursor_location(u32 x, u32 y) {
    u16 location = y * WIDTH + x;

    io::write<u8>(0x3D4, 0x0F);
    io::write<u8>(0x3D5, static_cast<u8>(location & 0xFF));

    io::write<u8>(0x3D4, 0x0E);
    io::write<u8>(0x3D5, static_cast<u8>((location >> 8) & 0xFF));
}

vga::Cursor vga::get_cursor_location() {
    u16 location = 0;

    io::write<u8>(0x3D4, 0x0F);
    location |= io::read<u8>(0x3D5);

    io::write<u8>(0x3D4, 0x0E);
    location |= io::read<u8>(0x3D5) << 8;

    return { location % WIDTH, location / WIDTH };
}

void vga::write(char c) {
    write(c, make_color(Color::White, Color::Black));
}

void vga::write(char c, u8 color) {
    if (c == '\n') {
        x = 0;
        y++;
    } else if (c == '\t') {
        x += 4;
    } else if (c == '\b') {
        if (x == 0 && y == 0) {
            return;
        }

        if (x > 0) {
            x--;
        } else {
            x = WIDTH - 1;
            y--;
        }

        // FIXME: I shouldn't be doing this because '\b' is a non-destructive backspace
        u16* buffer = reinterpret_cast<u16*>(ADDRESS);
        buffer[y * WIDTH + x] = make_vga_entry(' ', color);
    } else {
        u16* buffer = reinterpret_cast<u16*>(ADDRESS);
        buffer[y * WIDTH + x] = make_vga_entry(c, color);

        x++;
    }

    if (x >= WIDTH) {
        x = 0;
        y++;
    }

    if (y >= HEIGHT) {
        scroll();
    }

    set_cursor_location(x, y);
}

void vga::write(const char* str) {
    write(str, std::strlen(str));
}

void vga::write(const char* str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        write(str[i]);
    }
}

void vga::printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
            case 'c': {
                char c = static_cast<char>(va_arg(args, int));
                vga::write(c);

                break;
            }
            case 's': {
                const char* str = va_arg(args, const char*);
                vga::write(str);
                
                break;
            }
            case 'd': {
                int n = va_arg(args, int);
                char str[33];

                std::itoa(n, str, 10);
                vga::write(str);

                break;
            }
            case 'x': {
                unsigned int n = va_arg(args, unsigned int);
                char str[33];

                std::itoa(n, str, 16);
                vga::write(str);

                break;
            }
            case 'b': {
                int n = va_arg(args, int);
                char str[33];

                std::itoa(n, str, 2);
                vga::write(str);

                break;
            }
            case 'u': {
                unsigned int n = va_arg(args, unsigned int);
                char str[33];

                std::itoa(n, str, 10);
                vga::write(str);

                break;
            }
            case '*': {
                int width = va_arg(args, int);
                fmt++;

                if (*fmt != 's') {
                    break;
                }

                const char* str = va_arg(args, const char*);
                vga::write(str, width);
                
                break;
            }
            default:
                break;
            }
        } else {
            write(*fmt);
        }

        fmt++;
    }

    va_end(args);
}

void vga::clear() {
    u16* buffer = reinterpret_cast<u16*>(ADDRESS);

    for (u32 i = 0; i < WIDTH * HEIGHT; i++) {
        buffer[i] = make_vga_entry(' ', make_color(Color::White, Color::Black));
    }
}

void vga::scroll() {
    u16* buffer = reinterpret_cast<u16*>(ADDRESS);

    for (u32 i = 0; i < WIDTH * (HEIGHT - 1); i++) {
        buffer[i] = buffer[i + WIDTH];
    }

    for (u32 i = 0; i < WIDTH; i++) {
        buffer[(HEIGHT - 1) * WIDTH + i] = make_vga_entry(' ', make_color(Color::White, Color::Black));
    }

    x = 0;
    y = HEIGHT - 1;
}

}