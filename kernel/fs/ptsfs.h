#pragma once

#include <kernel/fs/filesystem.h>
#include <kernel/fs/inode.h>
#include <kernel/devices/device.h>
#include <kernel/tty/master.h>

namespace kernel::fs {

class PTSFS;

class PTSInode : public Inode {
public:
    PTSInode(PTSFS* fs, DeviceID id, u32 index) : m_fs(fs), m_device_id(id), m_index(index) {}

    ssize_t read(void*, size_t, size_t) const override { return 0; }
    ssize_t write(const void*, size_t, size_t) override { return 0; }

    void truncate(size_t) override {}

    mode_t mode() const override { return 0; }
    size_t size() const override { return 0; }

    bool is_character_device() const override { return m_index != 1; }
    bool is_directory() const override { return m_index == 1; }

    bool is_fifo() const override { return false; }
    bool is_block_device() const override { return false; }
    bool is_regular_file() const override { return false; }
    bool is_symlink() const override { return false; }
    bool is_unix_socket() const override { return false; }

    u32 major() const override;
    u32 minor() const override;

    struct stat stat() const override;

    void readdir(std::Function<IterationAction(const DirectoryEntry&)>) const override;
    RefPtr<Inode> lookup(StringView name) const override;

    ErrorOr<void> add_entry(String, RefPtr<Inode>) override { return Error(EROFS); }
    RefPtr<Inode> create_entry(String, mode_t, dev_t, uid_t, gid_t) override { return nullptr; }

    void flush() override {}

private:
    PTSFS* m_fs;

    DeviceID m_device_id;
    u32 m_index;
};

class PTSFS : public FileSystem {
public:
    fs::FileSystemID id() const override { return fs::FileSystemID::PtsFS; }
    StringView name() const override { return "ptsfs"; }

    void init();

    ino_t root() const override { return 1; }
    RefPtr<Inode> inode(ino_t id) override;

    static void register_pty(u32 pts);
    static void unregister_pty(u32 pts);

private:
    RefPtr<PTSInode> m_root;

};

}