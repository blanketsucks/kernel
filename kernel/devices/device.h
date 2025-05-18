#pragma once

#include <kernel/fs/file.h>

#include <std/memory.h>
#include <std/hash_map.h>
#include <std/queue.h>

namespace kernel::fs {
    class FileDescriptor;
}

namespace kernel {

struct DeviceID {
    u32 major;
    u32 minor;
};

enum class DeviceMajor : u32 {
    Generic = 1,
    Storage = 3,
    StoragePartition = 4,
    Input = 5,
    Audio = 6,
    GPU = 7,

    PTYMultiplexer = 99,
    MasterPTY = 100,
    SlavePTY = 101,
    VirtualTTY = 102,
};

struct DeviceEvent {
    enum : u8 {
        Added = 0,
        Removed = 1,
    };

    u32 major;
    u32 minor;
    u8 event_type;
    bool is_block_device;
}; 

class Device : public fs::File {
public:
    virtual ~Device() = default;
    Device(DeviceMajor major, u32 minor);

    DeviceMajor major() const { return m_major; }
    u32 minor() const { return m_minor; }

    DeviceID id() const { return { to_underlying(m_major), m_minor }; }

    static constexpr u32 encode(u32 major, u32 minor) { 
        return (minor & 0xFF) | (major << 8) | ((minor & ~0xFF) << 12);
    }

    static constexpr DeviceID decode(u32 id) {
        return { (id >> 8) & 0xFFF, (id & 0xFF) | ((id >> 12) & ~0xFF) };
    }

    static RefPtr<Device> get_device(DeviceMajor major, u32 minor);
    static Queue<DeviceEvent>& event_queue();
    static void add_device_event(Device*);

    virtual RefPtr<fs::FileDescriptor> open(int options);

    virtual ErrorOr<size_t> read(void* buffer, size_t size, size_t offset) override = 0;
    virtual ErrorOr<size_t> write(const void* buffer, size_t size, size_t offset) override = 0;

    size_t size() const override { return 0; }

    virtual bool is_character_device() const { return false; }
    virtual bool is_block_device() const { return false; }

protected:
    RefPtr<Device> as_ref() const;

private:
    DeviceMajor m_major = DeviceMajor::Generic;
    u32 m_minor = 0;
};

}