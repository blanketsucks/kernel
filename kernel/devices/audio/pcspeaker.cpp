#include <kernel/devices/audio/pcspeaker.h>
#include <kernel/io.h>

namespace kernel {

void PCSpeaker::beep(u32 frequency) {
    u32 div = 1193180 / frequency;

    io::write<u8>(0x43, 0xb6);
    io::write<u8>(0x42, static_cast<u8>(div));
    io::write<u8>(0x42, static_cast<u8>(div >> 8));

    u8 tmp = io::read<u8>(0x61);
    if (tmp != (tmp | 3)) {
        io::write<u8>(0x61, tmp | 3);
    }
}

void PCSpeaker::stop() {
    u8 tmp = io::read<u8>(0x61) & 0xFC;
    io::write<u8>(0x61, tmp);
}

}
