#pragma once

#include <kernel/common.h>
#include <kernel/devices/device.h>

namespace kernel::devices {

class CharacterDevice : public Device {
public:
    virtual ~CharacterDevice() = default;

    virtual size_t read(void* buffer, size_t size, size_t offset) override = 0;
    virtual size_t write(const void* buffer, size_t size, size_t offset) override = 0;

    bool is_character_device() const final override { return true; }

protected:
    CharacterDevice(u32 major, u32 minor) : Device(major, minor) {}
};

}