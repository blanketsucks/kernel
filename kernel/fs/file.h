#pragma once

#include <kernel/common.h>
#include <kernel/posix/sys/stat.h>
#include <std/result.h>
#include <std/memory.h>

namespace kernel {
    class Process;
};

namespace kernel::fs {

class Inode;

class File {
public:
    virtual ~File() = default;

    virtual struct stat stat() const { return {}; }

    virtual ErrorOr<size_t> read(void* buffer, size_t size, size_t offset) = 0;
    virtual ErrorOr<size_t> write(const void* buffer, size_t size, size_t offset) = 0;

    virtual size_t size() const = 0;

    virtual void close() { }

    virtual ErrorOr<void*> mmap(Process&, size_t, int) { return Error(ENODEV); }
    virtual ErrorOr<int> ioctl(unsigned, unsigned) { return Error(ENOTTY); }

    virtual ssize_t readdir(void*, size_t) { return -ENOTDIR; }
};

class InodeFile : public File {
public:
    InodeFile(RefPtr<Inode> inode) : m_inode(inode) {}

    Inode& inode() { return *m_inode; }
    Inode const& inode() const { return *m_inode; }

    struct stat stat() const override;

    ErrorOr<size_t> read(void* buffer, size_t size, size_t offset) override;
    ErrorOr<size_t> write(const void* buffer, size_t size, size_t offset) override;

    size_t size() const override;

    ErrorOr<void*> mmap(Process& process, size_t size, int prot) override;
    ssize_t readdir(void* buffer, size_t size) override;

private:
    RefPtr<Inode> m_inode;
};

}