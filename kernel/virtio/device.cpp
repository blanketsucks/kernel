#include <kernel/virtio/device.h>
#include <kernel/memory/manager.h>

#include <std/format.h>

namespace kernel::virtio {

template<> u8 Configuration::read<u8>(u32 off) const {
    VirtualAddress address = device->m_bars[bar];
    return *reinterpret_cast<u8*>(address + offset + off);
}

template<> u16 Configuration::read<u16>(u32 off) const {
    VirtualAddress address = device->m_bars[bar];
    return *reinterpret_cast<u16*>(address + offset + off);
}

template<> u32 Configuration::read<u32>(u32 off) const {
    VirtualAddress address = device->m_bars[bar];
    return *reinterpret_cast<u32*>(address + offset + off);
}

template<> void Configuration::write<u8>(u32 off, u8 value) {
    VirtualAddress address = device->m_bars[bar];
    *reinterpret_cast<u8*>(address + offset + off) = value;
}

template<> void Configuration::write<u16>(u32 off, u16 value) {
    VirtualAddress address = device->m_bars[bar];
    *reinterpret_cast<u16*>(address + offset + off) = value;
}

template<> void Configuration::write<u32>(u32 off, u32 value) {
    VirtualAddress address = device->m_bars[bar];
    *reinterpret_cast<u32*>(address + offset + off) = value;
}

template<> void Configuration::write<u64>(u32 off, u64 value) {
    VirtualAddress address = device->m_bars[bar];
    *reinterpret_cast<u64*>(address + offset + off) = value;
}

Device::Device(pci::Device device) : IRQHandler(device.interrupt_line()), m_pci_device(device) {
    this->initialize();

    this->enable_irq();
}

void Device::initialize() {
    m_pci_device.enable_bus_mastering();

    for (auto& cap : m_pci_device.capabilities()) {
        if (cap.id() != 0x09) {
            continue;
        }

        Configuration config;

        // FIXME: Check for sane values
        config.type = static_cast<Configuration::Type>(cap.read<u8>(0x3));
        config.bar = cap.read<u8>(0x4);
        config.offset = cap.read<u32>(0x8);
        config.length = cap.read<u32>(0xC);
        config.device = this;

        m_configurations.append(config);
    }

    for (auto& config : m_configurations) {
        pci::BARType type = m_pci_device.bar_type(config.bar);
        if (type == pci::BARType::IO) {
            // TODO: Support
            continue;
        }

        size_t size = m_pci_device.bar_size(config.bar);
        PhysicalAddress address = m_pci_device.bar(config.bar);

        auto* region = MM->map_physical_region(reinterpret_cast<void*>(address), size);
        m_bars[config.bar] = VirtualAddress { region };
    }
    
    auto* config = m_common_config = get_config(Configuration::Common);
    auto* notify = get_config(Configuration::Notify);
    
    this->reset();
    config->write<u8>(CommonConfig::DeviceStatus, DeviceStatus::Acknowledge | DeviceStatus::Driver);

    m_pci_device.enable_interrupts();
    m_notify_multiplier = notify->read<u32>(0x10);
}

void Device::post_init() {
    auto* config = m_common_config;

    u8 status = config->read<u8>(CommonConfig::DeviceStatus);
    config->write<u8>(CommonConfig::DeviceStatus, status | DeviceStatus::DriverOK);
}

u64 Device::features() {
    u64 features = 0;
    auto* config = m_common_config;

    config->write<u32>(CommonConfig::DeviceFeatureSelect, 0);
    features = config->read<u32>(CommonConfig::DeviceFeature);

    config->write<u32>(CommonConfig::DeviceFeatureSelect, 1);
    features |= (static_cast<u64>(config->read<u32>(CommonConfig::DeviceFeature)) << 32);

    return features;
}

ErrorOr<void> Device::set_accepted_features(u64 accepted) {
    u64 features = this->features();
    
    if (std::has_flag(features, VIRTIO_F_VERSION_1)) {
        accepted |= VIRTIO_F_VERSION_1;
    }
    
    this->set_features(accepted);
    
    auto* config = m_common_config;
    u8 status = config->read<u8>(CommonConfig::DeviceStatus);

    config->write<u8>(CommonConfig::DeviceStatus, status | DeviceStatus::FeaturesOK);
    if (!(config->read<u8>(CommonConfig::DeviceStatus) & DeviceStatus::FeaturesOK)) {
        return Error(EIO);
    }

    m_accepted_features = accepted;
    return {};
}

void Device::set_features(u64 features) {
    auto* config = m_common_config;

    config->write<u32>(CommonConfig::DriverFeatureSelect, 0);
    config->write<u32>(CommonConfig::DriverFeature, static_cast<u32>(features));

    config->write<u32>(CommonConfig::DriverFeatureSelect, 1);
    config->write<u32>(CommonConfig::DriverFeature, static_cast<u32>(features >> 32));
}

void Device::reset() {
    auto* config = m_common_config;

    config->write<u8>(CommonConfig::DeviceStatus, 0);
    while (config->read<u8>(CommonConfig::DeviceStatus) != 0) {
        // Wait for the device to be reset
    }
}

Queue& Device::queue(u16 index) {
    ASSERT(index < m_queues.size(), "Invalid queue index");
    return *m_queues[index];
}

ErrorOr<void> Device::setup_queues(u16 count) {
    auto* config = m_common_config;

    u16 num_queues = config->read<u16>(CommonConfig::NumQueues);
    if (!count) {
        count = num_queues;
    } else if (count > num_queues) {
        return Error(EINVAL);
    }

    for (u16 i = 0; i < count; ++i) {
        TRY(this->setup_queue(i));
    }

    for (u16 i = 0; i < count; ++i) {
        config->write<u16>(CommonConfig::QueueSelect, i);
        config->write<u16>(CommonConfig::QueueEnable, 1);
    }

    return {};
}

ErrorOr<void> Device::setup_queue(u16 index) {
    auto* config = m_common_config;

    config->write<u16>(CommonConfig::QueueSelect, index);
    u16 size = config->read<u16>(CommonConfig::QueueSize);
    if (size == 0) {
        return Error(ENXIO);
    }

    u16 notify_offset = config->read<u16>(CommonConfig::QueueNotifyOffset);    
    auto queue = Queue::create(size, notify_offset);
    
    config->write<u64>(CommonConfig::QueueDescriptorAddress, queue->get_physical_address(queue->descriptors()));
    config->write<u64>(CommonConfig::QueueDriverAddress, queue->get_physical_address(queue->driver()));
    config->write<u64>(CommonConfig::QueueDeviceAddress, queue->get_physical_address(queue->device()));
    
    m_queues.append(move(queue));
    return {};
}

ErrorOr<void> Device::notify(u16 index) {
    auto* notify = get_config(Configuration::Notify);
    if (index >= m_queues.size()) {
        return Error(EINVAL);
    }

    auto& queue = m_queues[index];
    notify->write<u16>(queue->notify_offset() * m_notify_multiplier, index);

    return {};
}

void Device::handle_irq() {
    auto* config = get_config(Configuration::ISR);
    u8 status = config->read<u8>(0x00);

    if (!status) {
        return;
    }

    if (status & ISRStatus::ConfigurationChange) {
        this->handle_config_change();
    }

    if (status & ISRStatus::UsedBufferNotification) {
        for (auto& queue : m_queues) {
            this->handle_queue_irq(*queue);
        }
    }
}


}