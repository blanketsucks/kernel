#include <kernel/fs/vfs.h>

#include <kernel/common.h>
#include <kernel/serial.h>

namespace kernel::fs {

bool VFS::mount_root(FileSystem* fs) {
    if (m_root) {
        return false;
    }

    auto inode = fs->inode(fs->root());
    if (!inode->is_directory()) {
        return false;
    }

    m_root = ResolvedInode::create("/", inode, nullptr);
    m_mounts.append(Mount(fs, m_root));

    return true;
}

ErrorOr<RefPtr<ResolvedInode>> VFS::resolve(StringView path, RefPtr<ResolvedInode> relative_to) {
    serial::printf("path: %*s\n", path.size(), path.data());
    if (path.empty()) {
        return Error(ENOENT);
    } else if (path == "/") {
        return m_root;
    }

    RefPtr<ResolvedInode> current = nullptr;
    if (relative_to && path[0] != '/') {
        current = relative_to;
    } else {
        if (path[0] == '/') path = path.substr(1);
        current = m_root;
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
        current = ResolvedInode::create(component, entry, current);
        auto* mount = this->find_mount(*current);

        if (mount) {
            auto* guest = mount->guest();

            auto root = guest->inode(guest->root());
            current = ResolvedInode::create(component, root, current);
        }
    }

    return current;
}

ErrorOr<RefPtr<File>> VFS::open(StringView, int, mode_t) {
    return RefPtr<File>(nullptr);
}

ErrorOr<void> VFS::mount(FileSystem* fs, RefPtr<ResolvedInode> target) {
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
    return {};
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