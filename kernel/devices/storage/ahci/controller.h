#pragma once

#include <kernel/common.h>
#include <kernel/pci/pci.h>
#include <kernel/arch/irq.h>
#include <kernel/devices/storage/controller.h>
#include <kernel/devices/storage/ahci/ahci.h>
#include <kernel/devices/storage/ahci/ports.h>

#include <std/array.h>
#include <std/memory.h>

namespace kernel {

class AHCIController : public StorageController, public IRQHandler {
public:
    static RefPtr<AHCIController> create(pci::Device);

    void set_pending_port(u32 port) {
        m_pending_ports |= (1 << port);
    }

    RefPtr<StorageDevice> device(size_t index) const override;
    size_t devices() const override;

private:
    AHCIController(pci::Address);

    template<typename T>
    void for_each_device(T&& callback) const {
        for (size_t i = 0; i < 32; i++) {
            auto& port = m_ports[i];
            if (!port) {
                continue;
            }

            auto device = port->device();
            if (!device) {
                continue;
            }

            callback(device);
        }
    }
    
    void handle_irq() override;

    ahci::HBAMemory* m_hba;
    Array<RefPtr<AHCIPort>, 32> m_ports;

    u32 m_pending_ports = 0;
};

}