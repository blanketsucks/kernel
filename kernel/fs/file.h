#pragma once

#include <kernel/common.h>
#include <kernel/posix/sys/stat.h>

#include <std/memory.h>

namespace kernel::fs {

class Inode;

class File {
public:
    virtual ~File() = default;

    virtual size_t read(void* buffer, size_t size, size_t offset) = 0;
    virtual size_t write(const void* buffer, size_t size, size_t offset) = 0;

    virtual size_t size() const = 0;
};

class InodeFile : public File {
public:
    InodeFile(RefPtr<Inode> inode) : m_inode(inode) {}

    Inode& inode() { return *m_inode; }
    Inode const& inode() const { return *m_inode; }

    size_t read(void* buffer, size_t size, size_t offset) override;
    size_t write(const void* buffer, size_t size, size_t offset) override;

    size_t size() const override;

private:
    RefPtr<Inode> m_inode;
};

}