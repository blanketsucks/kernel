#pragma once

#include <kernel/common.h>
#include <std/string.h>
#include <std/hash_map.h>
#include <std/memory.h>

#include <kernel/fs/filesystem.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/file.h>

namespace kernel::ramfs {

constexpr ino_t ROOT_INODE = 1;

enum InodeFlags {
    Directory = 1 << 0,
};

struct Inode : public fs::Inode {
public:
    virtual ~Inode() = default;

    static RefPtr<Inode> create(String name, int flags, ino_t parent);

    ssize_t read(void* buffer, size_t size, size_t offset) const override;
    ssize_t write(const void* buffer, size_t size, size_t offset) override;

    void truncate(size_t size) override;

    mode_t mode() const override { return 0; }
    size_t size() const override { return m_size; }

    bool is_fifo() const override { return false; }
    bool is_character_device() const override { return false; }
    bool is_directory() const override { return m_flags & InodeFlags::Directory; }
    bool is_block_device() const override { return false; }
    bool is_regular_file() const override { return !is_directory(); }
    bool is_symlink() const override { return false; }
    bool is_unix_socket() const override { return false; }

    u32 major() const override { return 0; }
    u32 minor() const override { return 0; }

    struct stat stat() const override;

   void readdir(std::Function<IterationAction(const fs::DirectoryEntry&)>) const override;
    RefPtr<fs::Inode> lookup(StringView name) const override;

    ErrorOr<void> add_entry(String name, RefPtr<fs::Inode> inode) override;
    RefPtr<fs::Inode> create_entry(String name, mode_t mode, dev_t dev, uid_t uid, gid_t gid) override;

    void flush() override;

private:
    Inode(ino_t id, String name, int flags, ino_t parent) : m_id(id), m_name(name), m_flags(flags), m_parent(parent) {}

    ino_t m_id = 0;

    String m_name;
    int m_flags = 0;

    void* m_data = nullptr;
    size_t m_size = 0;

    ino_t m_parent = 0;
    HashMap<String, RefPtr<Inode>> m_children;
};

class FileSystem : public fs::FileSystem {
public:
    static FileSystem* create();

    fs::FileSystemID id() const override { return fs::FileSystemID::RamFS; }
    StringView name() const override { return "ramfs"; }

    ino_t root() const override { return ROOT_INODE; }
    RefPtr<fs::Inode> inode(ino_t id) override;

private:
    FileSystem();

    RefPtr<Inode> m_root;
    HashMap<ino_t, RefPtr<Inode>> m_inodes;
};

}
