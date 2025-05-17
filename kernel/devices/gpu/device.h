#pragma once

#include <kernel/devices/block_device.h>
#include <kernel/devices/gpu/manager.h>
#include <kernel/devices/gpu/connector.h>

namespace kernel {

class GPUDevice : public BlockDevice {
public:

protected:
    GPUDevice() : BlockDevice(DeviceMajor::GPU, GPUManager::generate_device_minor(), 0) {}

    Vector<RefPtr<GPUConnector>> m_connectors;
};

}