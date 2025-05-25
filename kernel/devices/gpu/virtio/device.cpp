#include <kernel/devices/gpu/virtio/device.h>
#include <kernel/devices/gpu/virtio/virtio.h>
#include <kernel/devices/gpu/edid.h>
#include <kernel/devices/device.h>
#include <kernel/pci/pci.h>
#include <kernel/memory/manager.h>

#include <std/format.h>


namespace kernel {

using namespace virtio;

RefPtr<GPUDevice> VirtIOGPUDevice::create(pci::Device pci_device) {
    RefPtr<VirtIOGPUDevice> device = nullptr;
    if (!pci_device.is_virtio_device()) {
        return device;
    } else if (pci_device.device_id() != virtio::pci_device_type(DeviceType::GPUDevice)) {
        return device;
    }

    device = kernel::Device::create<VirtIOGPUDevice>(pci_device);

    auto result = device->initialize();
    if (result.is_err()) {
        return nullptr;
    }

    result = device->test();
    if (result.is_err()) {
        return nullptr;
    }

    return device;
}

VirtIOGPUDevice::VirtIOGPUDevice(pci::Device device) : virtio::Device(device) {}

ErrorOr<void> VirtIOGPUDevice::initialize() {
    TRY(this->setup_queues(2));

    u64 features = this->features();
    u64 accepted = 0;

    if (std::has_flag(features, VIRTIO_GPU_F_EDID)) {
        accepted |= VIRTIO_GPU_F_EDID;
    }

    TRY(this->set_accepted_features(accepted));
    this->post_init();

    m_command_buffer = reinterpret_cast<u8*>(MM->allocate_kernel_region(10 * PAGE_SIZE));
    m_device_config = get_config(Configuration::Device);

    m_num_scanouts = m_device_config->read<u32>(GPUDeviceConfig::Scanouts);

    GPUDisplayInfo* display_info = TRY(this->get_display_info());
    for (u32 i = 0; i < m_num_scanouts; i++) {
        auto& mode = display_info->modes[i];
        m_scanouts[i] = { i, mode.rect };
    }

    dbgln();
    dbgln("GPU VirtIO Device:");
    dbgln(" - Number of scanouts: {}", m_num_scanouts);
    dbgln(" - Supports VirGL: {}", bool(features & VIRTIO_GPU_F_VIRGL));
    dbgln(" - Supports EDID: {}", bool(features & VIRTIO_GPU_F_EDID));

    return {};
}

void VirtIOGPUDevice::populate_header(GPUControlHeader& header, GPUControlType type, u32 flags) {
    header.type = type;
    header.flags = flags;
    header.fence_id = 0;
    header.ctx_id = 0;
    header.padding = 0;
}

ErrorOr<void> VirtIOGPUDevice::test() {
    if (accepted_features() & VIRTIO_GPU_F_EDID) {
        GPUGetEDIDResponse* response = TRY(this->get_edid(0));
        EDID* edid = reinterpret_cast<EDID*>(response->edid);

        for (auto& timing : edid->standard_timings) {
            dbgln(" - Standard Timing: {} @ {}Hz", (timing.x_resolution + 31) * 8, timing.vertical_frequency + 60);
        }

        for (auto& timing : edid->timings) {
            if (timing.monitor_descriptor.descriptor != 0) {
                continue;
            } else if (timing.monitor_descriptor.type != 0xfc) {
                continue;
            }

            StringView text = { reinterpret_cast<const char*>(timing.monitor_descriptor.data), 13 };
            dbgln(" - Monitor Descriptor: {}", text);
        }
    }

    auto* fb = (u32*)MM->allocate_kernel_region(768 * 1024 * 4);
    PhysicalAddress address = MM->get_physical_address(fb);

    u32 id = TRY(this->create_resource_2d(GPUFormat::B8G8R8X8, 1024, 768));
    TRY(this->attach_resource_backing(id, address, 768 * 1024 * 4));

    TRY(this->set_resource_scanout(0, id, { 0, 0, 1024, 768 }));
    for (size_t i = 0; i < 1024 * 768; i++) {
        fb[i] = 0xffffffff;
    }

    TRY(this->transfer_to_host_2d(id, { 0, 0, 1024, 768 }, 0));
    TRY(this->resource_flush(id, { 0, 0, 1024, 768 }));

    return {};
}

void VirtIOGPUDevice::send_command(size_t request, size_t response) {
    auto& queue = this->queue(0);
    auto chain = queue.create_chain();

    PhysicalAddress address = MM->get_physical_address(m_command_buffer);

    chain.add_buffer(address, request, false);
    chain.add_buffer(address + request, response, true);

    chain.submit();
    this->notify(0);

    while (!queue.has_available_data()) {}
    queue.drain();
}

void VirtIOGPUDevice::handle_queue_irq(virtio::Queue&) {}

void VirtIOGPUDevice::handle_config_change() {
    u32 events = m_device_config->read<u32>(GPUDeviceConfig::EventsRead);
    if (events & 0x01) {
        m_device_config->write<u32>(GPUDeviceConfig::EventsClear, 0x01);
    }
}

ErrorOr<GPUDisplayInfo*> VirtIOGPUDevice::get_display_info() {
    auto buffer = this->get_command_buffer();

    auto* request = buffer.read<GPUControlHeader>();
    this->populate_header(*request, GPUControlType::GetDisplayInfo);

    this->send_command(sizeof(GPUControlHeader), sizeof(GPUDisplayInfo));

    auto* response = buffer.read<GPUDisplayInfo>();
    if (response->header.type != GPUControlType::RespOKDisplayInfo) {
        dbgln("get_display_info: Failed with response type {:#x}", response->header.type);
        return Error(EIO);
    }

    return response;
}

ErrorOr<GPUGetEDIDResponse*> VirtIOGPUDevice::get_edid(u32 scanout_id) {
    auto buffer = this->get_command_buffer();

    auto* request = buffer.read<GPUGetEDID>();
    this->populate_header(request->header, GPUControlType::GetEDID);

    request->scanout = scanout_id;
    this->send_command(sizeof(GPUGetEDID), sizeof(GPUGetEDIDResponse));

    auto* response = buffer.read<GPUGetEDIDResponse>();
    if (response->header.type != GPUControlType::RespOKEDID) {
        dbgln("get_edid: Failed with response type {:#x}", response->header.type);
        return Error(EIO);
    }

    return response;
}

ErrorOr<u32> VirtIOGPUDevice::create_resource_2d(GPUFormat format, u32 width, u32 height) {
    auto buffer = this->get_command_buffer();

    auto* request = buffer.read<GPUResourceCreate2D>();
    this->populate_header(request->header, GPUControlType::ResourceCreate2D);

    request->format = format;
    request->width = width;
    request->height = height;
    request->resource_id = this->allocate_resource();

    this->send_command(sizeof(GPUResourceCreate2D), sizeof(GPUControlHeader));
    
    auto* response = buffer.read<GPUControlHeader>();
    if (response->type != GPUControlType::RespOKNoData) {
        dbgln("create_resource_2d: Failed with response type {:#x}", response->type);
        return Error(EIO);
    }

    return request->resource_id;
}

ErrorOr<void> VirtIOGPUDevice::attach_resource_backing(u32 resource_id, PhysicalAddress address, size_t size) {
    auto buffer = this->get_command_buffer();

    auto* request = buffer.read<GPUResourceAttachBacking>();
    this->populate_header(request->header, GPUControlType::ResourceAttachBacking);

    size_t nr_entries = size / PAGE_SIZE;
    size_t header_size = sizeof(GPUResourceAttachBacking) + sizeof(GPUMemEntry) * nr_entries;

    request->nr_entries = nr_entries;
    for (size_t i = 0; i < nr_entries; i++) {
        auto& entry = request->entries[i];

        entry.address = address + (i * PAGE_SIZE);
        entry.length = PAGE_SIZE;
    }

    request->resource_id = resource_id;

    buffer.advance(sizeof(GPUMemEntry) * nr_entries);
    this->send_command(header_size, sizeof(GPUControlHeader));

    auto* response = buffer.read<GPUControlHeader>();
    if (response->type != GPUControlType::RespOKNoData) {
        dbgln("attach_resource_backing: Failed with response type {:#x}", response->type);
        return Error(EIO);
    }

    return {};
}

ErrorOr<void> VirtIOGPUDevice::set_resource_scanout(u32 scanout_id, u32 resource_id, GPURect rect) {
    auto buffer = this->get_command_buffer();

    auto* request = buffer.read<GPUSetScanout>();
    this->populate_header(request->header, GPUControlType::SetScanout);

    request->scanout_id = scanout_id;
    request->resource_id = resource_id;
    request->rect = rect;

    this->send_command(sizeof(GPUSetScanout), sizeof(GPUControlHeader));

    auto* response = buffer.read<GPUControlHeader>();
    if (response->type != GPUControlType::RespOKNoData) {
        dbgln("set_resource_scanout: Failed with response type {:#x}", response->type);
        return Error(EIO);
    }

    return {};
}

ErrorOr<void> VirtIOGPUDevice::transfer_to_host_2d(u32 resource_id, GPURect rect, u32 offset) {
    auto buffer = this->get_command_buffer();

    auto* request = buffer.read<GPUTransferToHost2D>();
    this->populate_header(request->header, GPUControlType::TransferToHost2D);

    request->resource_id = resource_id;
    request->rect = rect;
    request->offset = offset;

    this->send_command(sizeof(GPUTransferToHost2D), sizeof(GPUControlHeader));

    auto* response = buffer.read<GPUControlHeader>();
    if (response->type != GPUControlType::RespOKNoData) {
        dbgln("transfer_to_host_2d: Failed with response type {:#x}", response->type);
        return Error(EIO);
    }

    return {};
}

ErrorOr<void> VirtIOGPUDevice::resource_flush(u32 resource_id, GPURect rect) {
    auto buffer = this->get_command_buffer();

    auto* request = buffer.read<GPUResourceFlush>();
    this->populate_header(request->header, GPUControlType::ResourceFlush);

    request->resource_id = resource_id;
    request->rect = rect;

    this->send_command(sizeof(GPUResourceFlush), sizeof(GPUControlHeader));

    auto* response = buffer.read<GPUControlHeader>();
    if (response->type != GPUControlType::RespOKNoData) {
        dbgln("resource_flush: Failed with response type {:#x}", response->type);
        return Error(EIO);
    }

    return {};
}

}