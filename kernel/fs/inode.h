#pragma once

#include <kernel/common.h>
#include <kernel/posix/sys/types.h>
#include <kernel/posix/sys/stat.h>

#include <std/vector.h>
#include <std/memory.h>
#include <std/string.h>

namespace kernel::fs {

class FileSystem;

struct DirectoryEntry {
    enum Type : u8 {
        Unknown = 0,
        RegularFile = 1,
        Directory = 2,
        CharacterDevice = 3,
        BlockDevice = 4,
        FIFO = 5,
        Socket = 6,
        SymbolicLink = 7
    };

    ino_t inode;
    Type type;
    String name;
};

class Inode {
public:
    virtual ~Inode() = default;

    virtual size_t read(void* buffer, size_t size, size_t offset) const = 0;
    virtual size_t write(const void* buffer, size_t size, size_t offset) = 0;

    bool operator==(const Inode& other) const {
        return this->id() == other.id();
    }

    ino_t id() const { return m_id; }
    FileSystem& fs() const { return *m_fs; }

    virtual mode_t mode() const = 0;
    virtual size_t size() const = 0;

    virtual bool is_fifo() const = 0;
    virtual bool is_character_device() const = 0;
    virtual bool is_directory() const = 0;
    virtual bool is_block_device() const = 0;
    virtual bool is_regular_file() const = 0;
    virtual bool is_symlink() const = 0;
    virtual bool is_unix_socket() const = 0;

    bool is_device() const {
        return is_character_device() || is_block_device();
    }

    virtual u32 major() const = 0; // device major number
    virtual u32 minor() const = 0; // device minor number

    virtual struct stat stat() const = 0;

    virtual Vector<DirectoryEntry> readdir() const = 0;
    virtual RefPtr<Inode> lookup(StringView name) const = 0;

    virtual void add_entry(String name, RefPtr<Inode> inode) = 0;
    virtual RefPtr<Inode> create_entry(String name, mode_t mode, uid_t uid, gid_t gid) = 0;

protected:
    Inode() = default;
    Inode(FileSystem* fs, ino_t id) : m_fs(fs), m_id(id) {}

    FileSystem* m_fs;
    ino_t m_id;
};

class ResolvedInode {
public:
    ResolvedInode(
        String name, RefPtr<Inode> inode, RefPtr<ResolvedInode> parent
    ) : m_name(move(name)), m_parent(parent), m_inode(inode) {}

    static RefPtr<ResolvedInode> create(String name, RefPtr<Inode> inode, RefPtr<ResolvedInode> parent) {
        return RefPtr<ResolvedInode>(new ResolvedInode(move(name), inode, parent));
    }

    ResolvedInode* parent() { return m_parent.ptr(); }
    const ResolvedInode* parent() const { return m_parent.ptr(); }

    Inode& inode() { return *m_inode; }
    const Inode& inode() const { return *m_inode; }

    FileSystem& fs() { return m_inode->fs(); }
    const FileSystem& fs() const { return m_inode->fs(); }

    const String& name() const { return m_name; }

    String fullpath() const;

private:
    String m_name;

    RefPtr<ResolvedInode> m_parent;
    RefPtr<Inode> m_inode;
};

}