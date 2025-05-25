#pragma once

#include <kernel/pci/device.h>
#include <kernel/pci/controllers/controller.h>

#include <std/function.h>
#include <std/optional.h>
#include <std/memory.h>

namespace kernel {

class PCI {
public:
    static void initialize();
    static PCI* instance();

    static void enumerate(Function<void(const pci::Device&)>&& callback);

    static Optional<pci::Device> find_device(u16 vendor_id, u16 device_id, i16 prog_if = -1);

    template<typename T>
    T read(u32 domain, u8 bus, u8 device, u8 function, u32 offset) = delete;

    template<typename T>
    void write(u32 domain, u8 bus, u8 device, u8 function, u32 offset, T value) = delete;

private:
    void init();

    Vector<RefPtr<pci::Controller>> m_controllers;
    Vector<pci::Device> m_devices;
};

template<> u8 PCI::read<u8>(u32 domain, u8 bus, u8 device, u8 function, u32 offset);
template<> u16 PCI::read<u16>(u32 domain, u8 bus, u8 device, u8 function, u32 offset);
template<> u32 PCI::read<u32>(u32 domain, u8 bus, u8 device, u8 function, u32 offset);

template<> void PCI::write<u8>(u32 domain, u8 bus, u8 device, u8 function, u32 offset, u8 value);
template<> void PCI::write<u16>(u32 domain, u8 bus, u8 device, u8 function, u32 offset, u16 value);
template<> void PCI::write<u32>(u32 domain, u8 bus, u8 device, u8 function, u32 offset, u32 value);

}
