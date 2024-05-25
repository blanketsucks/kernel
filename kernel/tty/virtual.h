#pragma once

#include <kernel/tty/tty.h>

namespace kernel {

class VirtualTTY : public TTY {
public:
    VirtualTTY(u32 index);

protected:
    void on_write(const u8* buffer, size_t size) override;
    void echo(u8 byte) override;

};

}