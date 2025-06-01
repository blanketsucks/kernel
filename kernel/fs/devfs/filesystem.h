#pragma once

#include <kernel/common.h>
#include <kernel/devices/device.h>

#include <std/result.h>
#include <std/string.h>
#include <std/hash_map.h>
#include <std/function.h>

namespace kernel::devfs {

enum class FormatStyle {
    Default = 0,
    ASCII,
    Callback
};

class DeviceRange {
public:
    DeviceRange() = default;
    DeviceRange(String name, DeviceMajor major, FormatStyle format_style) : m_name(move(name)), m_major(major), m_format_style(format_style) {}
    DeviceRange(DeviceMajor major, Function<String(DeviceEvent)> callback) 
        : m_major(major), m_callback(move(callback)), m_format_style(FormatStyle::Callback) {}

    DeviceMajor major() const { return m_major; }
    String const& name() const { return m_name; }

    Function<String(DeviceEvent)> const& callback() const {
        return m_callback;
    }

    FormatStyle format_style() const { return m_format_style; }

private:
    String m_name;
    DeviceMajor m_major;

    Function<String(DeviceEvent)> m_callback = nullptr;

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

StringView get_device_path(u32 major, u32 minor);

void register_device_range(String name, DeviceMajor major, FormatStyle format_style = FormatStyle::Default);
void register_device_range(DeviceMajor major, Function<String(DeviceEvent)> callback);

}