#pragma once

#include <kernel/devices/character_device.h>

#include <std/vector.h>

namespace kernel {

class AudioDevice : public CharacterDevice {
public:
    virtual ~AudioDevice() = default;

    virtual ErrorOr<void> set_sample_rate(u16 sample_rate) = 0;
    virtual u16 sample_rate() const = 0;

    virtual ErrorOr<size_t> read(void* buffer, size_t size, size_t offset) override = 0;
    virtual ErrorOr<size_t> write(const void* buffer, size_t size, size_t offset) override = 0;

    virtual bool can_read(fs::FileDescriptor const&) const override = 0;
    virtual bool can_write(fs::FileDescriptor const&) const override = 0;

    ErrorOr<int> ioctl(unsigned request, unsigned arg) override;

protected:
    AudioDevice();
};

}