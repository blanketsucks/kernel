#pragma once

#include "std/types.h"
#include <kernel/common.h>

namespace kernel::io {

template<typename T> T read(u16 port) = delete;

template<> u8 read<u8>(u16 port);
template<> u16 read<u16>(u16 port);
template<> u32 read<u32>(u16 port);

template<typename T> void write(u16 port, T data) = delete;

template<> void write<u8>(u16 port, u8 data);
template<> void write<u16>(u16 port, u16 data);
template<> void write<u32>(u16 port, u32 data);

void wait();
void wait(u32 us);

class Port {
public:
    Port() = default;
    Port(u16 port) : m_port(port) {}

    u16 value() const { return m_port; }
    Port offset(u16 offset) const { return Port(m_port + offset); }

    template<typename T> T read() const {
        return io::read<T>(m_port);
    }

    template<typename T> T read(u16 offset) const {
        return io::read<T>(m_port + offset);
    }

    template<typename T> void write(T data) const {
        io::write<T>(m_port, data);
    }

    template<typename T> void write(u16 offset, T data) const {
        io::write<T>(m_port + offset, data);
    }

private:
    u16 m_port;
};

}