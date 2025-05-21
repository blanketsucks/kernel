#include "kernel/posix/fcntl.h"
#include <kernel/fs/vfs.h>

#include <kernel/devices/device.h>
#include <kernel/common.h>
#include <kernel/serial.h>

#include <std/format.h>

namespace kernel::fs {

static VFS s_vfs_instance = {};

VFS* VFS::instance() {
    return &s_vfs_instance;
}

bool VFS::mount_root(FileSystem* fs) {
    if (m_root) {
        return false;
    }

    auto inode = fs->inode(fs->root());
    if (!inode->is_directory()) {
        return false;
    }

    m_root = ResolvedInode::create("/", fs, inode, nullptr);
    m_mounts.append(Mount(fs, m_root));

    return true;
}

ErrorOr<RefPtr<ResolvedInode>> VFS::resolve(StringView path, RefPtr<ResolvedInode>* parent, RefPtr<ResolvedInode> relative_to) {
    if (path.empty()) {
        return Error(ENOENT);
    } else if (path == "/") {
        return m_root;
    }

    RefPtr<ResolvedInode> current = nullptr;
    if (relative_to && path[0] != '/') {
        current = relative_to;
    } else {
        if (path[0] == '/') {
            path = path.substr(1);
        }

        current = m_root;
    }

    FileSystem* fs = current->fs();

    if (parent) {
        *parent = current;
    }

    while (!path.empty()) {
        if (!current->inode().is_directory()) {
            return Error(ENOTDIR);
        }

        size_t index = path.find('/');
        StringView component;

        if (index == StringView::npos) {
            component = path;
            path = {};
        } else {
            component = path.substr(0, index);
            path = path.substr(index + 1);
        }

        
        if (component == ".") {
            continue;
        } else if (component == "..") {
            if (current->parent()) {
                current = current->parent();
            }
            
            continue;
        }
        
        auto entry = current->inode().lookup(component);
        if (!entry) {
            return Error(ENOENT);
        }

        // TODO: Handle symbolic links
        current = ResolvedInode::create(component, fs, entry, current);
        auto* mount = this->find_mount(*current);

        if (mount) {
            auto* guest = mount->guest();
            fs = guest;

            auto root = guest->inode(guest->root());
            current = ResolvedInode::create(component, fs, root, current);
        }

        if (parent) {
            *parent = current;
        }
    }

    return current;
}

ErrorOr<RefPtr<FileDescriptor>> VFS::open(StringView path, int options, mode_t, RefPtr<ResolvedInode> relative_to) {
    if (path.empty()) {
        return Error(ENOENT);
    } else if ((options & O_DIRECTORY) && (options & O_CREAT)) {
        return Error(EINVAL);
    }

    
    // TODO: Handle O_CREAT and O_EXCL
    auto resolved = TRY(this->resolve(path, nullptr, relative_to));
    auto& inode = resolved->inode();

    if (inode.is_directory() && (options & O_DIRECTORY) == 0) {
        return Error(EISDIR);
    } else if (!inode.is_directory() && (options & O_DIRECTORY)) {
        return Error(ENOTDIR);
    }
    
    if (inode.is_device()) {
        auto device = Device::get_device(static_cast<DeviceMajor>(inode.major()), inode.minor());
        if (!device) {
            return Error(ENODEV);
        }

        return device->open(options);
    }

    auto file = RefPtr<InodeFile>::make(resolved->m_inode);
    auto fd = RefPtr<FileDescriptor>::make(file, options);

    fd->set_path(path);
    return move(fd);
}

ErrorOr<void> VFS::mknod(StringView path, mode_t mode, dev_t dev, RefPtr<ResolvedInode> relative_to) {
    if (path.empty()) {
        return Error(ENOENT);
    }

    RefPtr<ResolvedInode> parent;
    auto result = this->resolve(path, &parent, relative_to);

    if (!result.is_err()) {
        return Error(EEXIST);
    } else if (!parent) {
        return Error(ENOENT);
    } else if (result.error().code() != ENOENT) {
        return result.release_error();
    }

    auto& inode = parent->inode();

    StringView basename;
    size_t index = path.rfind('/');
    if (index == StringView::npos) {
        basename = path;
    } else {
        basename = path.substr(index + 1);
    }

    inode.create_entry(basename, mode, dev, 0, 0); // TODO: uid/gid
    return {};
}

ErrorOr<Mount*> VFS::mount(FileSystem* fs, RefPtr<ResolvedInode> target) {
    if (!target->inode().is_directory()) {
        return Error(ENOTDIR);
    }

    for (auto& mount : m_mounts) {
        auto& host = mount.host();
        if (host.inode() == target->inode() && host.fs() == target->fs()) {
            return Error(EBUSY);
        }
    }

    m_mounts.append(Mount(fs, target));
    return &m_mounts.last();
}

Mount const* VFS::find_mount(ResolvedInode const& inode) const {
    for (auto& mount : m_mounts) {
        auto& host = mount.host();
        if (host.inode() == inode.inode() && host.fs() == inode.fs()) {
            return &mount;
        }
    }

    return nullptr;
}

}