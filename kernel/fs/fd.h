#pragma once

#include <kernel/common.h>
#include <kernel/fs/file.h>

#include <kernel/posix/fcntl.h>
#include <kernel/posix/sys/types.h>


namespace kernel::fs {

// TODO: Implement this
class FileDescriptor {
public:
    FileDescriptor() = default;

    off_t offset() const { return m_offset; }
    i32 options() const { return m_options; }

    size_t read(void* buffer, size_t size);
    size_t write(const void* buffer, size_t size);

    bool is_readable() const;
    bool is_writable() const;

    void seek(off_t offset, int whence);

private:
    off_t m_offset;

    i32 m_options;
};

}