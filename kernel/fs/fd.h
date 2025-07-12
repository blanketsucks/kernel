#pragma once

#include <kernel/common.h>
#include <kernel/fs/file.h>
#include <kernel/posix/fcntl.h>
#include <kernel/posix/sys/types.h>

#include <std/string.h>
#include <std/memory.h>

namespace kernel {
    class Device;
}

namespace kernel::fs {

class FileDescriptor {
public:
    FileDescriptor(RefPtr<File> file, int options) : m_file(file), m_offset(0), m_options(options) {};
    ~FileDescriptor();

    static RefPtr<FileDescriptor> create(RefPtr<File> file, int options) {
        return RefPtr<FileDescriptor>::make(move(file), options);
    }

    size_t size() const { return m_file->size(); }
    String const& path() const { return m_path; }

    void set_path(String path) { m_path = move(path); }

    const File* file() const { return m_file.ptr(); }
    File* file() { return m_file.ptr(); }

    struct stat stat() const { return m_file->stat(); }

    off_t offset() const { return m_offset; }
    int options() const { return m_options; }

    bool is_readable() const;
    bool is_writable() const;
    
    ErrorOr<size_t> read(void* buffer, size_t size);
    ErrorOr<size_t> write(const void* buffer, size_t size);

    void seek(off_t offset, int whence);

    void close();

    ErrorOr<void*> mmap(Process& process, size_t size, int prot);
    ErrorOr<int> ioctl(unsigned request, unsigned arg);

private:
    friend class Process;

    RefPtr<File> m_file;

    off_t m_offset = 0;
    int m_options = 0;

    String m_path;
};

}