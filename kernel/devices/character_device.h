#pragma once

#include <kernel/common.h>
#include <kernel/devices/device.h>

namespace kernel {

class CharacterDevice : public Device {
public:
    virtual ~CharacterDevice() = default;

    virtual ssize_t read(void* buffer, size_t size, size_t offset) override = 0;
    virtual ssize_t write(const void* buffer, size_t size, size_t offset) override = 0;

    bool is_character_device() const final override { return true; }

protected:
    CharacterDevice(DeviceMajor major, u32 minor) : Device(major, minor) {}
};

}