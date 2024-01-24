#pragma once

#include <kernel/common.h>
#include <kernel/devices/character.h>

namespace kernel::devices {

class ZeroDevice : public CharacterDevice {
public:
    ZeroDevice() : CharacterDevice(1, 5) {}

    size_t read(void* buffer, size_t size, size_t offset) override;
    size_t write(const void* buffer, size_t size, size_t offset) override;
};

}