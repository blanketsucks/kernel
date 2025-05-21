#pragma once

#include <kernel/common.h>
#include <std/string.h>
#include <std/hash_map.h>
#include <std/memory.h>

#include <kernel/devices/device.h>
#include <kernel/fs/filesystem.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/file.h>

namespace kernel::ramfs {

constexpr ino_t ROOT_INODE = 1;

class FileSystem;

struct Inode : public fs::Inode {
public:
    virtual ~Inode() = default;

    static RefPtr<Inode> create(FileSystem*, String name, mode_t mode, dev_t dev, ino_t parent);

    ssize_t read(void* buffer, size_t size, size_t offset) const override;
    ssize_t write(const void* buffer, size_t size, size_t offset) override;

    void truncate(size_t size) override;

    mode_t mode() const override { return m_mode; }
    size_t size() const override { return m_size; }

    bool is_fifo() const override { return S_ISFIFO(m_mode); }
    bool is_character_device() const override { return S_ISCHR(m_mode); }
    bool is_directory() const override { return S_ISDIR(m_mode); }
    bool is_block_device() const override { return S_ISBLK(m_mode); }
    bool is_regular_file() const override { return S_ISREG(m_mode); }
    bool is_symlink() const override { return S_ISLNK(m_mode); }
    bool is_unix_socket() const override { return S_ISSOCK(m_mode); }

    u32 major() const override { return Device::decode(m_dev).major; }
    u32 minor() const override { return Device::decode(m_dev).minor; }

    struct stat stat() const override;

    void readdir(std::Function<IterationAction(const fs::DirectoryEntry&)>) const override;
    RefPtr<fs::Inode> lookup(StringView name) const override;

    ErrorOr<void> add_entry(String name, RefPtr<fs::Inode> inode) override;
    RefPtr<fs::Inode> create_entry(String name, mode_t mode, dev_t dev, uid_t uid, gid_t gid) override;

    void flush() override;

private:
    Inode(FileSystem* fs, ino_t id, String name, mode_t mode, dev_t dev, ino_t parent) : m_fs(fs), m_id(id), m_name(name), m_mode(mode), m_dev(dev), m_parent(parent) {}

    FileSystem* m_fs = nullptr;
    ino_t m_id = 0;

    String m_name;

    mode_t m_mode;
    dev_t m_dev = 0;
    
    void* m_data = nullptr;
    size_t m_size = 0;

    ino_t m_parent = 0;
    HashMap<String, RefPtr<Inode>> m_children;
};

class FileSystem : public fs::FileSystem {
public:
    static FileSystem* create() {
        return new FileSystem();
    }

    fs::FileSystemID id() const override { return fs::FileSystemID::RamFS; }
    StringView name() const override { return "ramfs"; }

    ino_t allocate_inode() { return m_next_inode++;}

    ino_t root() const override { return ROOT_INODE; }
    RefPtr<fs::Inode> inode(ino_t id) override;

private:
    FileSystem();

    RefPtr<Inode> m_root;
    HashMap<ino_t, RefPtr<Inode>> m_inodes;

    ino_t m_next_inode = ROOT_INODE + 1;
};

}
