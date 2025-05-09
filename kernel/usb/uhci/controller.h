#pragma once

#include <kernel/usb/uhci/uhci.h>
#include <kernel/arch/io.h>
#include <kernel/pci/pci.h>

namespace kernel::usb {

class UHCIController {
public:
    static UHCIController* create();

private:
    UHCIController(pci::Address);

    io::Port m_port;
};

}