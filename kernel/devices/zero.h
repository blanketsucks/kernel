#pragma once

#include <kernel/common.h>
#include <kernel/devices/character_device.h>

namespace kernel {

class ZeroDevice : public CharacterDevice {
public:
    ZeroDevice() : CharacterDevice(DeviceMajor::Generic, 2) {}

    ssize_t read(void* buffer, size_t size, size_t offset) override;
    ssize_t write(const void* buffer, size_t size, size_t offset) override;
};

}