#pragma once

#include <kernel/common.h>
#include <kernel/fs/inode.h>

#include <std/hash_map.h>
#include <kernel/posix/sys/types.h>

namespace kernel::fs {

enum class FileSystemID : u8 {
    RamFS = 0x01,
    Ext2FS = 0x02,
    PtsFS = 0x03
};

class FileSystem {
public:
    virtual ~FileSystem() = default;

    bool operator==(const FileSystem& other) const {
        return this->id() == other.id();
    }

    virtual FileSystemID id() const = 0;
    virtual StringView name() const = 0;

    virtual ino_t root() const = 0;
    virtual RefPtr<Inode> inode(ino_t id) = 0;

    void add_inode(ino_t id, RefPtr<Inode> inode) { m_inodes.set(id, inode); }

protected:
    HashMap<ino_t, RefPtr<Inode>> m_inodes;
};

};