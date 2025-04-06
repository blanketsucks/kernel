#pragma once

#include <kernel/pci.h>
#include <kernel/devices/storage/controller.h>
#include <kernel/devices/storage/ide/pata.h>

#include <std/array.h>

namespace kernel {

class IDEController : public StorageController {
public:
    static RefPtr<IDEController> create();
    static RefPtr<IDEController> create(pci::Device address);

    RefPtr<StorageDevice> device(size_t index) const override;
    RefPtr<StorageDevice> device(ata::Channel channel, ata::Drive drive) const;

    size_t devices() const override;

private:
    IDEController(pci::Address address) : m_address(address) {}

    void enumerate();

    pci::Address m_address;
    Array<RefPtr<PATADevice>, 4> m_devices;
};

}