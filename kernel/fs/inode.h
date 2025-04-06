#pragma once

#include <kernel/common.h>
#include <kernel/posix/sys/types.h>
#include <kernel/posix/sys/stat.h>

#include <std/vector.h>
#include <std/memory.h>
#include <std/string.h>
#include <std/function.h>

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

    DirectoryEntry(ino_t inode, Type type, String name) : inode(inode), type(type), name(move(name)) {}
};

class Inode {
public:
    virtual ~Inode() = default;

    virtual ssize_t read(void* buffer, size_t size, size_t offset) const = 0;
    virtual ssize_t write(const void* buffer, size_t size, size_t offset) = 0;

    virtual void truncate(size_t size) = 0;

    bool operator==(const Inode& other) const {
        return this->id() == other.id();
    }

    ino_t id() const { return m_id; }

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

    virtual void readdir(std::Function<IterationAction(const fs::DirectoryEntry&)>) const = 0;

    virtual RefPtr<Inode> lookup(StringView name) const = 0;

    virtual ErrorOr<void> add_entry(String name, RefPtr<Inode> inode) = 0;
    virtual RefPtr<Inode> create_entry(String name, mode_t mode, uid_t uid, gid_t gid) = 0;

    virtual void flush() = 0;

protected:
    Inode() = default;
    Inode(ino_t id) : m_id(id) {}

    ino_t m_id;
};

class ResolvedInode {
public:
    ResolvedInode(
        String name, FileSystem* fs, 
        RefPtr<Inode> inode, RefPtr<ResolvedInode> parent
    ) : m_name(move(name)), m_fs(fs), m_parent(parent), m_inode(inode) {}

    static RefPtr<ResolvedInode> create(String name, FileSystem* fs, RefPtr<Inode> inode, RefPtr<ResolvedInode> parent) {
        return RefPtr<ResolvedInode>(new ResolvedInode(move(name), fs, inode, parent));
    }

    ResolvedInode* parent() { return m_parent.ptr(); }
    const ResolvedInode* parent() const { return m_parent.ptr(); }

    Inode& inode() { return *m_inode; }
    const Inode& inode() const { return *m_inode; }

    FileSystem* fs() { return m_fs; }
    const FileSystem* fs() const { return m_fs; }

    String name() const { return m_name; }

    String fullpath() const;

private:
    friend class VFS;

    String m_name;
    FileSystem* m_fs;

    RefPtr<ResolvedInode> m_parent;
    RefPtr<Inode> m_inode;
};

}