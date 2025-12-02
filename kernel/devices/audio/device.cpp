#include <kernel/devices/audio/device.h>
#include <kernel/devices/audio/manager.h>
#include <kernel/process/process.h>
#include <kernel/posix/sys/ioctl.h>

namespace kernel {

AudioDevice::AudioDevice() : CharacterDevice(DeviceMajor::Audio, AudioManager::generate_device_minor()) {}

ErrorOr<int> AudioDevice::ioctl(unsigned request, unsigned arg) {
    auto* process = Process::current();
    switch (request) {
        // FIXME: Support the rest (SOUNDCARD_SET_VOLUME, SOUNDCARD_GET_VOLUME)
        case SOUNDCARD_GET_SAMPLE_RATE: {
            int* argp = reinterpret_cast<int*>(arg);
            process->validate_write(argp, sizeof(int));

            *argp = this->sample_rate();
            return 0;
        }
        case SOUNDCARD_SET_SAMPLE_RATE: {
            TRY(this->set_sample_rate(static_cast<u16>(arg)));
            return 0;
        }
        default:
            return Error(EINVAL);
    }
}

}