#pragma once

#include <kernel/common.h>
#include <kernel/devices/character_device.h>

namespace kernel {

class NullDevice : public CharacterDevice {
public:
    NullDevice() : CharacterDevice(DeviceMajor::Generic, 1) {}

    ssize_t read(void* buffer, size_t size, size_t offset) override;
    ssize_t write(const void* buffer, size_t size, size_t offset) override;
};

}