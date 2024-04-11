#include <kernel/serial.h>
#include <kernel/io.h>

#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_NOFLOAT
#include <std/stb_sprintf.h>

#include <std/string.h>

constexpr u16 SERIAL_PORT = 0x3F8;

namespace kernel::serial {

static bool s_initialized = false;

// Directly taken from https://wiki.osdev.org/Serial_Ports#Example_Code
bool init() {
    io::write<u8>(SERIAL_PORT + 1, 0x00);    // Disable all interrupts
    io::write<u8>(SERIAL_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    io::write<u8>(SERIAL_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    io::write<u8>(SERIAL_PORT + 1, 0x00);    //                  (hi byte)
    io::write<u8>(SERIAL_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    io::write<u8>(SERIAL_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    io::write<u8>(SERIAL_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    io::write<u8>(SERIAL_PORT + 4, 0x1E);    // Set in loopback mode, test the serial chip
    io::write<u8>(SERIAL_PORT + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

    // Check if serial is faulty (i.e: not same byte as sent)
    if (io::read<u8>(SERIAL_PORT + 0) != 0xAE) {
        return false;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    io::write<u8>(SERIAL_PORT + 4, 0x0F);
    s_initialized = true;

    return true;
}

bool is_initialized() { return s_initialized; }

bool is_ready_to_send() {
    return (io::read<u8>(SERIAL_PORT + 5) & 0x20);
}

bool is_transmit_empty() {
    return (io::read<u8>(SERIAL_PORT + 5) & 0x20);
}

void putc(char c) {
    if (!s_initialized) {
        serial::init();
    }

    while (!is_transmit_empty());
    io::write<u8>(SERIAL_PORT, c);
}

void write(const char* str, size_t len) {
    for (u32 i = 0; i < len; i++) {
        putc(str[i]);
    }

}

void write(const char* str) {
    write(str, std::strlen(str));
}

void printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buffer[4096];
    stbsp_vsnprintf(buffer, 4096, fmt, args);

    write(buffer);

    va_end(args);
}

}