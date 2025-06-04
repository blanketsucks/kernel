#pragma once

#include <kernel/devices/block_device.h>
#include <kernel/devices/gpu/manager.h>
#include <kernel/devices/gpu/connector.h>

namespace kernel {

class GPUDevice : public BlockDevice {
public:
    Vector<RefPtr<GPUConnector>>& connectors() { return m_connectors; }
    Vector<RefPtr<GPUConnector>> const& connectors() const { return m_connectors; }

    ErrorOr<bool> read_blocks(void*, size_t, size_t) override { return Error(EIO); }
    ErrorOr<bool> write_blocks(const void*, size_t, size_t) override { return Error(EIO); }
    size_t max_io_block_count() const override { return 0; }

    ErrorOr<int> ioctl(unsigned request, unsigned arg) override;

protected:
    GPUDevice() : BlockDevice(DeviceMajor::GPU, GPUManager::generate_device_minor(), 0) {}

    Vector<RefPtr<GPUConnector>> m_connectors;

    ErrorOr<GPUConnector*> get_connector(int id) const;
};

}