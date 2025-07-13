#pragma once

#include <kernel/common.h>

#include <kernel/fs/fd.h>
#include <kernel/fs/file.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/filesystem.h>

namespace kernel::fs {

class Mount {
public:
    Mount(FileSystem* fs, RefPtr<ResolvedInode> host) : m_fs(fs), m_host(host) {}

    RefPtr<ResolvedInode> host() const { return m_host; }
    FileSystem* guest() const { return m_fs; }

private:
    FileSystem* m_fs;
    RefPtr<ResolvedInode> m_host;
};

class VFS {
public:
    VFS() = default;

    static VFS* instance();

    ResolvedInode const& root() const { return *m_root; }

    ErrorOr<RefPtr<ResolvedInode>> resolve(StringView path, RefPtr<ResolvedInode>* parent = nullptr, RefPtr<ResolvedInode> relative_to = nullptr);

    ErrorOr<RefPtr<FileDescriptor>> open(StringView path, int options, mode_t mode, RefPtr<ResolvedInode> relative_to = nullptr);
    ErrorOr<void> remove(StringView path, RefPtr<ResolvedInode> relative_to = nullptr);
    ErrorOr<void> mknod(StringView path, mode_t mode, dev_t dev, RefPtr<ResolvedInode> relative_to = nullptr);
    ErrorOr<void> mkdir(StringView path, mode_t mode, RefPtr<ResolvedInode> relative_to = nullptr);

    bool mount_root(FileSystem* fs);
    ErrorOr<Mount*> mount(FileSystem* fs, RefPtr<ResolvedInode> target);

    Mount const* find_mount(ResolvedInode const& inode) const;

private:
    RefPtr<ResolvedInode> m_root;
    Vector<Mount> m_mounts;
};

ALWAYS_INLINE inline VFS* vfs() {
    return VFS::instance();
}

}