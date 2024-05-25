#include <kernel/devices/ac97.h>

namespace kernel::devices {

AC97Device* AC97Device::create() {
    pci::Address address = {};
    pci::enumerate([&address](pci::Device device) {
        if (device.is_audio_device()) {
            address = device.address;
        }
    });

    if (!address.value) {
        return nullptr;
    }

    return new AC97Device(address);
}

AC97Device::AC97Device(pci::Address address) : CharacterDevice(4, 0) {
    m_audio_mixer_port = address.bar0() & ~1;
    m_audio_bus_port = address.bar1() & ~1;

    address.set_bus_master(true);
    address.set_interrupt_line(true);
}

ssize_t AC97Device::read(void*, size_t, size_t) {
    return 0;
}

ssize_t AC97Device::write(const void*, size_t, size_t) {
    return 0;
}

}