#pragma once

#include <kernel/common.h>
#include <kernel/fs/file.h>
#include <kernel/posix/fcntl.h>
#include <kernel/posix/sys/types.h>

#include <std/memory.h>

namespace kernel::fs {

class FileDescriptor {
public:
    FileDescriptor(RefPtr<File> file, int flags) : m_file(file), m_offset(0), m_flags(flags) {};

    size_t size() const { return m_file->size(); }

    const File* file() const { return m_file.ptr(); }
    File* file() { return m_file.ptr(); }

    off_t offset() const { return m_offset; }
    int flags() const { return m_flags; }

    size_t read(void* buffer, size_t size);
    size_t write(const void* buffer, size_t size);

    bool is_readable() const;
    bool is_writable() const;

    void seek(off_t offset, int whence);

private:
    RefPtr<File> m_file;

    off_t m_offset = 0;
    int m_flags = 0;
};

}