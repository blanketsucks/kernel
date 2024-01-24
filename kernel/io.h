#pragma once

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

}