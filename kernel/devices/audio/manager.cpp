#include <kernel/devices/audio/manager.h>
#include <kernel/devices/audio/ac97/device.h>
#include <kernel/fs/devfs/filesystem.h>

namespace kernel {

static AudioManager s_instance;
static u32 s_audio_device_minor = 0;

u32 AudioManager::generate_device_minor() {
    return s_audio_device_minor++;
}

AudioManager* AudioManager::instance() {
    return &s_instance;
}

void AudioManager::initialize() {
    s_instance.enumerate();

    devfs::register_device_range("snd", DeviceMajor::Audio);
}

void AudioManager::enumerate() {
    PCI::enumerate([this](pci::Device device) {
        if (!device.is_audio_device()) {
            return;
        }

        RefPtr<AudioDevice> audio_device = AC97Device::create(device);
        if (audio_device) {
            m_devices.append(audio_device);
        }
    });
}

}