#pragma once

#include <kernel/devices/character_device.h>
#include <kernel/pci.h>

namespace kernel::devices {

struct BufferDescriptor {
    u32 address;
    u16 samples;

    u16 unused : 16;
    u8 last_entry : 1;
    u8 interrupt_on_completion : 1;
} PACKED;

class AC97Device : public CharacterDevice {
public:
    enum AudioMixerRegisters : u16 {
        Reset = 0x00,
        MasterVolume = 0x02,
        AuxVolume = 0x04,
        MicrophoneVolume = 0x0E,
        PCMVolume = 0x18,
        InputDevice = 0x1A,
        InputGain = 0x1C,
        MicrophoneGain = 0x1E,
        ExtendedCapabilties = 0x2A,
        SampleRate = 0x2C,
    };

    enum AudioBusRegisters : u16 {
        NABMPCMIn = 0x00,
        NABMPCMOut = 0x10,
        NABMMicrophone = 0x20,
        GlobalControl = 0x2C,
        GlobalStatus = 0x2E,
    };

    static AC97Device* create();

    ssize_t read(void* buffer, size_t size, size_t offset) override;
    ssize_t write(const void* buffer, size_t size, size_t offset) override;

private:
    AC97Device(pci::Address address);

    u32 m_audio_mixer_port;
    u32 m_audio_bus_port;
};

}