#include <kernel/serial.h>
#include <kernel/io.h>

#include <std/cstring.h>

namespace kernel::serial {

static COMPort s_com1(COM1);
static bool s_initialized = false;

// Directly taken from https://wiki.osdev.org/Serial_Ports#Example_Code
bool COMPort::init() {
    write_reg(InterruptEnable, 0x00);    // Disable all interrupts
    write_reg(LineControl, 0x80);        // Enable DLAB (set baud rate divisor)
    write_reg(BaudRateDivisorLSB, 0x03); // Set divisor to 3 (lo byte) 38400 baud
    write_reg(BaudRateDivisorMSB, 0x00); //                  (hi byte)
    write_reg(LineControl, 0x03);        // 8 bits, no parity, one stop bit
    write_reg(FIFOControl, 0xC7);        // Enable FIFO, clear them, with 14-byte threshold
    write_reg(ModemControl, 0x0B);       // IRQs enabled, RTS/DSR set
    write_reg(ModemControl, 0x1E);       // Set in loopback mode, test the serial chip
    write_reg(Data, 0xAE);               // Test serial chip (send byte 0xAE and check if serial returns same byte)

    // Check if serial is faulty (i.e: not same byte as sent)
    if (read_reg(Data) != 0xAE) {
        return false;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    write_reg(ModemControl, 0x0F);
    return true;
}

void COMPort::write(char c) {
    while (!this->is_transmit_empty()) {}
    this->write_reg(Data, c);
}

char COMPort::read() {
    while (!this->is_data_ready()) {}
    return this->read_reg(Data);
}

void COMPort::write_reg(Register reg, u8 value) {
    io::write<u8>(m_port + static_cast<u16>(reg), value);
}

u8 COMPort::read_reg(Register reg) {
    return io::read<u8>(m_port + static_cast<u16>(reg));
}

bool COMPort::is_transmit_empty() {
    return (this->read_reg(Register::LineStatus) & 0x20);
}

bool COMPort::is_data_ready() {
    return (this->read_reg(Register::LineStatus) & 1);
}

bool init() {
    s_initialized = s_com1.init();
    return s_initialized;
}

void putc(char c) {
    if (!s_initialized) {
        serial::init();
    }

    s_com1.write(c);
}

void write(const char* str, size_t len) {
    for (u32 i = 0; i < len; i++) {
        putc(str[i]);
    }
}

void write(const char* str) {
    write(str, std::strlen(str));
}

}