#pragma once

#include <kernel/fs/file.h>
#include <std/hash_map.h>

namespace kernel::devices {

struct DeviceID {
    u32 major;
    u32 minor;
};

class Device : public fs::File {
public:
    virtual ~Device() = default;
    Device(u32 major, u32 minor) : m_major(major), m_minor(minor) {}

    u32 major() const { return m_major; }
    u32 minor() const { return m_minor; }

    static constexpr u32 encode(u32 major, u32 minor) { 
        return (minor & 0xFF) | (major << 8) | ((minor & ~0xFF) << 12);
    }

    static constexpr DeviceID decode(u32 id) {
        return { (id >> 8) & 0xFFF, (id & 0xFF) | ((id >> 12) & ~0xFF) };
    }

    virtual size_t read(void* buffer, size_t size, size_t offset) override = 0;
    virtual size_t write(const void* buffer, size_t size, size_t offset) override = 0;

    virtual bool is_character_device() const { return false; }
    virtual bool is_block_device() const { return false; }
private:
    u32 m_major = 0;
    u32 m_minor = 0;
};

class DeviceManager {
public:
    static void init();
    static DeviceManager* instance();

    HashMap<u32, Device*> const& devices() const { return m_devices; }

    bool register_device(Device* device);
    Device* get_device(u32 major, u32 minor);

private:
    HashMap<u32, Device*> m_devices;

    static DeviceManager* s_instance;
};


}