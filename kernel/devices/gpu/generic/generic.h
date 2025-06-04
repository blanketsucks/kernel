#pragma once

#include <kernel/common.h>
#include <kernel/devices/gpu/device.h>
#include <std/format.h>

namespace kernel {

class GenericGPUDevice;

class GenericGPUConnector : public GPUConnector {
public:
    static RefPtr<GenericGPUConnector> create(GenericGPUDevice* device) {
        return RefPtr<GenericGPUConnector>(new GenericGPUConnector(device));
    }

    ErrorOr<Resolution> get_resolution() const override;
    ErrorOr<void*> map_framebuffer(Process*) override;
    ErrorOr<void> flush() override;

private:
    GenericGPUConnector(GenericGPUDevice* device) : GPUConnector(0), m_device(device) {}

    GenericGPUDevice* m_device;
};

class GenericGPUDevice : public GPUDevice {
public:
    static RefPtr<GenericGPUDevice> create_from_boot();
    static RefPtr<GenericGPUDevice> create(PhysicalAddress framebuffer, i32 width, i32 height, i32 pitch, i32 bpp);

    PhysicalAddress framebuffer() const { return m_framebuffer; }

    i32 width() const { return m_width; }
    i32 height() const { return m_height; }
    i32 pitch() const { return m_pitch; }
    i32 bpp() const { return m_bpp; }

private:
    friend class Device;

    GenericGPUDevice(PhysicalAddress framebuffer, i32 width, i32 height, i32 pitch, i32 bpp);

    PhysicalAddress m_framebuffer;

    i32 m_width;
    i32 m_height;
    i32 m_pitch;
    i32 m_bpp;
};

}