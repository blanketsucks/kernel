#pragma once

#include <kernel/fs/file.h>

#include <std/memory.h>
#include <std/hash_map.h>

namespace kernel::fs {
    class FileDescriptor;
}

namespace kernel {

struct DeviceID {
    u32 major;
    u32 minor;
};

class Device : public fs::File {
public:
    virtual ~Device() = default;
    Device(u32 major, u32 minor);

    u32 major() const { return m_major; }
    u32 minor() const { return m_minor; }

    DeviceID id() const { return { m_major, m_minor }; }

    static constexpr u32 encode(u32 major, u32 minor) { 
        return (minor & 0xFF) | (major << 8) | ((minor & ~0xFF) << 12);
    }

    static constexpr DeviceID decode(u32 id) {
        return { (id >> 8) & 0xFFF, (id & 0xFF) | ((id >> 12) & ~0xFF) };
    }

    static Device* get_device(u32 major, u32 minor);

    virtual RefPtr<fs::FileDescriptor> open(int options);

    virtual ssize_t read(void* buffer, size_t size, size_t offset) override = 0;
    virtual ssize_t write(const void* buffer, size_t size, size_t offset) override = 0;

    size_t size() const override { return 0; }

    virtual bool is_character_device() const { return false; }
    virtual bool is_block_device() const { return false; }
private:
    u32 m_major = 0;
    u32 m_minor = 0;
};

}