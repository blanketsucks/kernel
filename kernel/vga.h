#pragma once

#include <kernel/common.h>
#include <std/enums.h>

namespace kernel::vga {

struct Cursor {
    u32 x;
    u32 y;
};

constexpr u32 ADDRESS = 0xB8000;

constexpr u32 WIDTH = 80;
constexpr u32 HEIGHT = 25;

enum class Color : u8 {
    Black = 0,
    Blue = 1,
    Green = 2,
    Cyan = 3,
    Red = 4,
    Magenta = 5,
    Brown = 6,
    LightGray = 7,
    DarkGray = 8,
    LightBlue = 9,
    LightGreen = 10,
    LightCyan = 11,
    LightRed = 12,
    Pink = 13,
    Yellow = 14,
    White = 15,
};

constexpr u8 make_color(Color fg, Color bg) {
    return to_underlying(fg) | to_underlying(bg) << 4;
}

constexpr u16 make_vga_entry(char c, u8 color) {
    return static_cast<u16>(c) | static_cast<u16>(color) << 8;
}

void enable_cursor(u8 start, u8 end);
void disable_cursor();

void set_cursor_location(u32 x, u32 y);
Cursor get_cursor_location();

void clear();

void putc(char c);

void write(char c);
void write(char c, u8 color);
void write(const char* str);
void write(const char* str, size_t len);

void printf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

void scroll();

}