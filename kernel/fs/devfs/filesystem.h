#pragma once

#include <kernel/common.h>
#include <kernel/devices/device.h>

#include <std/result.h>
#include <std/string.h>
#include <std/hash_map.h>

namespace kernel::devfs {

enum class FormatStyle {
    Default = 0,
    ASCII
};

class DeviceRange {
public:
    DeviceRange() = default;
    DeviceRange(String name, DeviceMajor major, FormatStyle format_style) : m_name(move(name)), m_major(major), m_format_style(format_style) {}

    DeviceMajor major() const { return m_major; }
    String const& name() const { return m_name; }

    FormatStyle format_style() const { return m_format_style; }

private:
    String m_name;
    DeviceMajor m_major;

    FormatStyle m_format_style = FormatStyle::Default;
};

class Subsystem {
public:
    Subsystem(String name) : m_name(move(name)) {}

    String const& name() const { return m_name; }

    void add_range(String name, DeviceMajor major, FormatStyle format_style = FormatStyle::Default) {
        m_ranges.set(static_cast<u32>(major), DeviceRange(name, major, format_style));
    }

    DeviceRange* get_range(u32 major) {
        auto iterator = m_ranges.find(major);
        if (iterator == m_ranges.end()) {
            return nullptr;
        }

        return &iterator->value;
    }

private:
    String m_name;
    HashMap<u32, DeviceRange> m_ranges;
};

ErrorOr<void> mkdir(StringView path, mode_t mode);
ErrorOr<void> mknod(StringView path, mode_t mode, dev_t dev);

void init();
ErrorOr<void> mount();

Subsystem& create_subsystem(String name);
void register_device_range(String name, DeviceMajor major, FormatStyle format_style = FormatStyle::Default);

}