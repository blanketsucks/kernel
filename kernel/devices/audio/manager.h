#pragma once

#include <kernel/common.h>
#include <kernel/devices/audio/device.h>

namespace kernel {

class AudioManager {
public:
    AudioManager() = default;
    static AudioManager* instance();

    static void initialize();

    static u32 generate_device_minor();

private:
    void enumerate();

    Vector<RefPtr<AudioDevice>> m_devices;
};

}