#pragma once

#include <kernel/common.h>

namespace kernel::serial {

enum COMPortNumber {
    COM1 = 0x3F8,
    COM2 = 0x2F8,
    COM3 = 0x3E8,
    COM4 = 0x2E8,
};

class COMPort {
public:
    enum Register {
        Data = 0,            // with DLAB = 0
        InterruptEnable = 1, // with DLAB = 0

        BaudRateDivisorLSB = 0, // with DLAB = 1
        BaudRateDivisorMSB = 1, // with DLAB = 1

        InterruptIdentification = 2,
        FIFOControl = 2,
        LineControl = 3,
        ModemControl = 4,
        LineStatus = 5,
        ModemStatus = 6,
        Scratch = 7,
    };

    COMPort(u16 port) : m_port(port) {}

    u16 port() const { return m_port; }

    bool init();

    void write(char c);
    char read();

    void write_reg(Register, u8 value);
    u8 read_reg(Register);

    bool is_transmit_empty();
    bool is_data_ready();

private:
    u16 m_port;
};

bool init();
bool is_initialized();

void putc(char c);

void write(const char* str, size_t len);
void write(const char* str);

}