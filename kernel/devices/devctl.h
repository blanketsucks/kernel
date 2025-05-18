#pragma once

#include <kernel/devices/character_device.h>

namespace kernel {

class DeviceControl : public CharacterDevice {
public:
    static DeviceControl* create();

    ErrorOr<size_t> read(void* buffer, size_t size, size_t offset) override;
    ErrorOr<size_t> write(const void*, size_t, size_t) override {
        return Error(ENOTSUP);
    }

private:
    DeviceControl() : CharacterDevice(DeviceMajor::Generic, 3) {}
};

}