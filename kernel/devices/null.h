#pragma once

#include <kernel/common.h>
#include <kernel/devices/character_device.h>

namespace kernel {

class NullDevice : public CharacterDevice {
public:
    static NullDevice* create();

    ErrorOr<size_t> read(void* buffer, size_t size, size_t offset) override;
    ErrorOr<size_t> write(const void* buffer, size_t size, size_t offset) override;

private:
    friend class Device;

    NullDevice() : CharacterDevice(DeviceMajor::Generic, 1) {}
};

}