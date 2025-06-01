#include <kernel/fs/devfs/filesystem.h>
#include <kernel/fs/ramfs/filesystem.h>
#include <kernel/process/process.h>
#include <kernel/process/scheduler.h>
#include <kernel/fs/vfs.h>

#include <std/format.h>
#include <std/time.h>

namespace kernel::devfs {

// FIXME: The current way we handle devices doesn't allow for devices like /dev/sda1 to be created.

static RefPtr<fs::ResolvedInode> s_root;
static fs::FileSystem* s_fs = nullptr;

static Vector<Subsystem> s_subsystems;
static HashMap<u32, DeviceRange> s_ranges;

static HashMap<u32, String> s_nodes;

static void poll();
static void handle_device_event(DeviceEvent event);

void init() {
    s_fs = ramfs::FileSystem::create();

    // We don't to mount the filesystem here because we might not have a root fs yet.
    // But we still want to be able to create nodes inside the devfs so we manually create a ResolvedInode to act
    // as the root of the devfs for VFS operations.
    auto root = s_fs->inode(s_fs->root());
    s_root = fs::ResolvedInode::create("/dev", s_fs, root, nullptr);

    mkdir("pts", 0);

    mknod("null", S_IFCHR, Device::encode(1, 1));
    mknod("zero", S_IFCHR, Device::encode(1, 2));
    mknod("ptmx", S_IFCHR, Device::encode(99, 0));

    auto* process = Process::create_kernel_process("Device Poller", poll);
    Scheduler::add_process(process);
}

StringView get_device_path(u32 major, u32 minor) {
    auto iterator = s_nodes.find(Device::encode(major, minor));
    if (iterator == s_nodes.end()) {
        return {};
    }

    return iterator->value;
}

ErrorOr<void> mount() {
    auto* vfs = fs::vfs();

    auto target = TRY(vfs->resolve("/dev"));
    TRY(vfs->mount(s_fs, target));

    return {};
}

ErrorOr<void> mkdir(StringView path, mode_t mode) {
    auto* vfs = fs::vfs();
    TRY(vfs->mkdir(path, mode, s_root));

    return {};
}

ErrorOr<void> mknod(StringView path, mode_t mode, dev_t dev) {
    auto* vfs = fs::vfs();
    TRY(vfs->mknod(path, mode, dev, s_root));

    s_nodes.set(dev, path);
    return {};
}

Subsystem& create_subsystem(String name) {
    auto* vfs = fs::vfs();
    vfs->mkdir(name, 0755, s_root);

    s_subsystems.append(Subsystem(move(name)));
    return s_subsystems.last();
}

void register_device_range(String name, DeviceMajor major, FormatStyle format_style) {
    s_ranges.set(static_cast<u32>(major), DeviceRange(name, major, format_style));
}

void register_device_range(DeviceMajor major, Function<String(DeviceEvent)> callback) {
    s_ranges.set(static_cast<u32>(major), DeviceRange(major, move(callback)));
}

static void poll() {
    using namespace std::time_literals;

    auto* thread = Thread::current();
    while (true) {
        thread->sleep(100_ms);
        auto& events = Device::event_queue();
        if (events.empty()) {
            continue;
        }

        while (!events.empty()) {
            auto event = events.dequeue();
            handle_device_event(event);
        }
    }
}

static void handle_device_event(DeviceEvent event) {
    auto iterator = s_ranges.find(event.major);

    DeviceRange* range = nullptr;
    String basepath;

    if (iterator == s_ranges.end()) {
        for (auto& subsystem : s_subsystems) {
            range = subsystem.get_range(event.major);
            if (!range) {
                continue;
            }

            basepath = format("{}/{}", subsystem.name(), range->name());
            break;
        }

        if (basepath.empty()) {
            return;
        }
    } else {
        range = &iterator->value;
        basepath = range->name();
    }

    mode_t mode = 0777;
    if (event.is_block_device) {
        mode |= S_IFBLK;
    } else {
        mode |= S_IFCHR;
    }

    String name = basepath;
    if (range->format_style() == FormatStyle::ASCII) {
        name.append('a' + event.minor);
    } else if (range->format_style() == FormatStyle::Callback) {
        name = range->callback()(event);
    } else {
        name = format("{}{}", basepath, event.minor);
    }

    switch (event.event_type) {
        case DeviceEvent::Added: {
            mknod(name, mode, Device::encode(event.major, event.minor));
            break;
        }
    }
}

}