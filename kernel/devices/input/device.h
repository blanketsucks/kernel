#pragma once

#include <kernel/common.h>
#include <kernel/devices/character_device.h>

namespace kernel {

class InputDevice : public CharacterDevice {
public:

protected:
    InputDevice(DeviceMajor major, u32 minor) : CharacterDevice(major, minor) {}

};


}