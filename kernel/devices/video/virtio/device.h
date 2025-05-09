#pragma once

#include <kernel/virtio/virtio.h>
#include <kernel/virtio/device.h>
#include <kernel/devices/block_device.h>
#include <kernel/devices/video/virtio/virtio.h>

#include <std/bytes_buffer.h>

namespace kernel {

class VirtIOGPUDevice : public virtio::Device, public BlockDevice {
public:
    static ErrorOr<VirtIOGPUDevice*> create();
    
    bool read_blocks(void*, size_t, size_t) override { return -EINVAL; }
    bool write_blocks(const void*, size_t, size_t) override { return -EINVAL; }

    size_t max_io_block_count() const override { return 0; }

    ErrorOr<void> test();

private:
    VirtIOGPUDevice(pci::Device device);

    ErrorOr<void> initialize();

    void handle_queue_irq(virtio::Queue&) override;
    void handle_config_change() override;

    u32 allocate_resource() {
        return m_resource_id++;
    }

    ErrorOr<u32> create_resource_2d(virtio::GPUFormat format, u32 width, u32 height);
    ErrorOr<void> attach_resource_backing(u32 resource_id, PhysicalAddress address, size_t size);
    ErrorOr<void> set_resource_scanout(u32 scanout_id, u32 resource_id, virtio::GPURect rect);
    ErrorOr<void> transfer_to_host_2d(u32 resource_id, virtio::GPURect rect, u32 offset);
    ErrorOr<void> resource_flush(u32 resource_id, virtio::GPURect rect);

    void populate_header(virtio::GPUControlHeader& header, virtio::GPUControlType type, u32 flags = 0);

    void send_command(size_t request, size_t response);
    std::BytesBuffer get_command_buffer() { return std::BytesBuffer(m_command_buffer, 10 * PAGE_SIZE); }

    u8* m_command_buffer = nullptr;
    u32 m_resource_id = 1;

    virtio::Configuration* m_device_config = nullptr;
};

}