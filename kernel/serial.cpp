#include <kernel/serial.h>
#include <kernel/arch/io.h>
#include <kernel/sync/spinlock.h>
#include <kernel/sync/lock.h>
#include <kernel/arch/processor.h>

#include <std/cstring.h>

namespace kernel::serial {

static COMPort s_com1(COM1);
static bool s_initialized = false;

static SpinLock s_lock;

// Directly taken from https://wiki.osdev.org/Serial_Ports#Example_Code
bool COMPort::init() {
    m_port.write<u8>(InterruptEnable, 0x00);    // Disable all interrupts
    m_port.write<u8>(LineControl, 0x80);        // Enable DLAB (set baud rate divisor)
    m_port.write<u8>(BaudRateDivisorLSB, 0x03); // Set divisor to 3 (lo byte) 38400 baud
    m_port.write<u8>(BaudRateDivisorMSB, 0x00); //                  (hi byte)
    m_port.write<u8>(LineControl, 0x03);        // 8 bits, no parity, one stop bit
    m_port.write<u8>(FIFOControl, 0xC7);        // Enable FIFO, clear them, with 14-byte threshold
    m_port.write<u8>(ModemControl, 0x0B);       // IRQs enabled, RTS/DSR set
    m_port.write<u8>(ModemControl, 0x1E);       // Set in loopback mode, test the serial chip
    m_port.write<u8>(Data, 0xAE);               // Test serial chip (send byte 0xAE and check if serial returns same byte)

    // Check if serial is faulty (i.e: not same byte as sent)
    if (m_port.read<u8>(Data) != 0xAE) {
        return false;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    m_port.write<u8>(ModemControl, 0x0F);
    return true;
}

void COMPort::write(char c) {
    while (!this->is_transmit_empty()) {}
    m_port.write<u8>(Data, c);
}

void COMPort::write(const char* str, size_t len) {
    if (Processor::are_interrupts_initialized()) {
        ScopedLock lock(s_lock);
        while (len--)
            this->write(*str++);

        return;
    }
        
    while (len--)
        this->write(*str++);
}

char COMPort::read() {
    while (!this->is_data_ready()) {}
    return m_port.read<u8>(Data);
}

bool COMPort::is_transmit_empty() {
    return (m_port.read<u8>(Register::LineStatus) & 0x20);
}

bool COMPort::is_data_ready() {
    return (m_port.read<u8>(Register::LineStatus) & 1);
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
    s_com1.write(str, len);
}

void write(const char* str) {
    s_com1.write(str, std::strlen(str));
}

}