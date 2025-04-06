#include <kernel/devices/storage/ahci/controller.h>
#include <kernel/devices/storage/ahci/sata.h>

#include <std/format.h>
#include <std/hash_map.h>

namespace kernel {

using namespace ahci;

static HashMap<u32, StringView> VERSIONS = {
    { AHCI_0_95, "0.95" },
    { AHCI_1_0,  "1.0"  },
    { AHCI_1_1,  "1.1"  },
    { AHCI_1_2,  "1.2"  },
    { AHCI_1_3,  "1.3"  },
    { AHCI_1_31, "1.31" },
};

AHCIController* AHCIController::create(pci::Device device) {
    if (!device.is_sata_controller()) {
        return nullptr;
    }

    return new AHCIController(device.address);
}

AHCIController* AHCIController::create() {
    pci::Address address;
    pci::enumerate([&address](pci::Device device) {
        if (device.is_sata_controller()) {
            address = device.address;
        }
    });

    if (address.is_null()) {
        return nullptr;
    }

    return new AHCIController(address);
}

AHCIController::AHCIController(pci::Address address) : IRQHandler(address.interrupt_line()) {
    m_hba = reinterpret_cast<HBAMemory*>(MM->map_physical_region(reinterpret_cast<void*>(address.bar5()), PAGE_SIZE * 2));
    
    m_hba->global_host_control = GlobalHostControl::HR; // Initiate a reset
    while (m_hba->global_host_control & GlobalHostControl::HR) {}
    
    m_hba->global_host_control = GlobalHostControl::AE | GlobalHostControl::IE;
    m_hba->interrupt_status = 0; // Clear all interrupts
    
    bool supports_64bit = m_hba->capabilities & Capabilities::S64A;
    bool supports_bios_handoff = m_hba->host_capabilities_extended & ExtendedCapabilities::BOH;
    
    if (supports_bios_handoff) {
        m_hba->bios_os_handoff_control_status.oos = 1;
        while (m_hba->bios_os_handoff_control_status.bos) {}
    }

    address.set_bus_master(true);
    address.set_interrupt_line(true);

    u32 pi = m_hba->ports_implemented;
    m_ports.fill({});

    dbgln("SATA Controller ({}:{}:{}):", address.bus, address.device, address.function);
    dbgln(" - Version: {}", VERSIONS.get(m_hba->version).value_or("Unknown"));
    dbgln(" - Supports 64-bit: {}", supports_64bit);
    dbgln(" - Supports BIOS Handoff: {}", supports_bios_handoff);

    for (size_t index = 0; index < 32; index++) {
        if (!(pi & (1 << index))) {
            continue;
        }

        auto port = AHCIPort::create(&m_hba->ports[index], index, this);
        m_ports[index] = port;
        
        port->initialize();
    }
}

void AHCIController::handle_interrupt(arch::InterruptRegisters*) {
    auto& pending = m_hba->interrupt_status;
    for (size_t index = 0; index < 32; index++) {
        if (!(pending & (1 << index))) {
            continue;
        }

        auto& port = m_ports[index];
        port->handle_interrupt();

        pending |= (1 << index);
    }
}

RefPtr<StorageDevice> AHCIController::device(size_t index) const {
    Array<RefPtr<StorageDevice>, 32> devices;
    size_t count = 0;

    for_each_device([&](RefPtr<SATADevice> device) {
        devices[count++] = device;
    });

    if (index >= count) {
        return nullptr;
    }

    return devices[index];
}

size_t AHCIController::devices() const {
    size_t count = 0;
    for_each_device([&count](RefPtr<SATADevice>) {
        count++;
    });

    return count;
}

}