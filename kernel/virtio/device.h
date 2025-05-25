#pragma once

#include <kernel/virtio/queue.h>
#include <kernel/pci/device.h>
#include <kernel/arch/irq.h>

#include <std/vector.h>
#include <std/array.h>
#include <std/result.h>

namespace kernel::virtio {

class Device;

struct Configuration {
    enum Type {
        Common = 1,
        Notify = 2,
        ISR = 3,
        Device = 4,
        PCI = 5,
    };

    Type type;
    u8 bar;
    u32 offset;
    u32 length;

    class Device* device;

    template<typename T> T read(u32 offset) const = delete;
    template<typename T> void write(u32 offset, T value) = delete;
};

class Device : public IRQHandler {
public:
    Device(pci::Device);

    ErrorOr<void> setup_queues(u16 count);
    void post_init();

    Queue& queue(u16 index);
    ErrorOr<void> notify(u16 index);

    u64 features();
    u64 accepted_features() const { return m_accepted_features; }

    ErrorOr<void> set_accepted_features(u64 accepted);
    
protected:
    virtual void handle_queue_irq(Queue&) = 0;
    virtual void handle_config_change() = 0;

    Configuration* get_config(Configuration::Type type) {
        for (auto& config : m_configurations) {
            if (config.type == type) {
                return &config;
            }
        }
        return nullptr;
    }

private:
    friend struct Configuration;
    
    void handle_irq() override;
    
    void initialize();
    void reset();
    
    ErrorOr<void> setup_queue(u16 index);
    void set_features(u64 features);

    pci::Device m_pci_device;

    Vector<Configuration> m_configurations;
    Vector<OwnPtr<Queue>> m_queues;
    
    Array<VirtualAddress, 6> m_bars;

    Configuration* m_common_config = nullptr;

    u32 m_notify_multiplier = 0;
    u64 m_accepted_features = 0;
};

template<> u8 Configuration::read<u8>(u32 offset) const;
template<> u16 Configuration::read<u16>(u32 offset) const;
template<> u32 Configuration::read<u32>(u32 offset) const;

template<> void Configuration::write<u8>(u32 offset, u8 value);
template<> void Configuration::write<u16>(u32 offset, u16 value);
template<> void Configuration::write<u32>(u32 offset, u32 value);
template<> void Configuration::write<u64>(u32 offset, u64 value);

}