#pragma once

#include <kernel/common.h>
#include <kernel/fs/inode.h>

#include <std/hash_map.h>
#include <kernel/posix/sys/types.h>

namespace kernel::fs {

class FileSystem {
public:
    virtual ~FileSystem() = default;

    bool operator==(const FileSystem& other) const {
        return this->id() == other.id();
    }

    virtual u8 id() const = 0;
    virtual StringView name() const = 0;

    virtual ino_t root() const = 0;
    virtual RefPtr<Inode> inode(ino_t id) = 0;

    void add_inode(ino_t id, RefPtr<Inode> inode) { m_inodes.set(id, inode); }

protected:
    HashMap<ino_t, RefPtr<Inode>> m_inodes;
};

};