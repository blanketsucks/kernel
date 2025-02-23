#include <kernel/tty/virtual.h>
#include <std/format.h>

namespace kernel {

VirtualTTY::VirtualTTY(u32 index) : TTY(69, index) {}

void VirtualTTY::on_write(const u8* data, size_t size) {
    StringView view(reinterpret_cast<const char*>(data), size);
    dbg("{}", view);
}

void VirtualTTY::echo(u8) {}

}