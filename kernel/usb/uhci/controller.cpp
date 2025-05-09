#include <kernel/usb/uhci/controller.h>
#include <kernel/memory/manager.h>
#include <kernel/arch/io.h>

#include <std/format.h>

namespace kernel::usb {

using namespace uhci;

UHCIController* UHCIController::create() {
    pci::Address address;
    pci::enumerate([&address](pci::Device device) {
        if (device.is_usb_controller()) {
            u8 prog_if = device.address().prog_if();
            if (prog_if != 0x00) {
                return;
            }

            address = device.address();
        }
    });

    if (!address.value()) {
        return nullptr;
    }

    auto controller = new UHCIController(address);
    if (!controller) {
        return nullptr;
    }

    return controller;
}

UHCIController::UHCIController(pci::Address address) {
    dbgln();

    address.set_bus_master(true);

    m_port = address.bar(4) & ~1;

    m_port.write<u16>(IORegister::USBCMD, Command::HCRESET);
    while (m_port.read<u16>(IORegister::USBCMD) & Command::HCRESET) {
        asm volatile("pause");
    }

    io::wait(10 * 1000);
    m_port.write<u16>(IORegister::USBCMD, Command::RS | Command::MPS);

    dbgln();
}

}