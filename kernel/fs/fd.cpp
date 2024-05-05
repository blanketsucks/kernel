#include <kernel/fs/fd.h>
#include <kernel/serial.h>
#include <kernel/posix/errno.h>
#include <kernel/posix/unistd.h>

namespace kernel::fs {

size_t FileDescriptor::read(void* buffer, size_t size) {
    if (!this->is_readable()) {
        return -EBADF;
    }

    size_t nread = m_file->read(buffer, size, m_offset);
    m_offset += nread;

    return nread;
}

size_t FileDescriptor::write(const void* buffer, size_t size) {
    if (!this->is_writable()) {
        return -EBADF;
    }

    size_t nwritten = m_file->write(buffer, size, m_offset);
    m_offset += nwritten;

    return nwritten;
}

bool FileDescriptor::is_readable() const {
    return m_flags & O_RDONLY || m_flags & O_RDWR;
}

bool FileDescriptor::is_writable() const {
    return m_flags & O_WRONLY || m_flags & O_RDWR;
}

void FileDescriptor::seek(off_t offset, int whence) {
    switch (whence) {
        case SEEK_SET:
            m_offset = offset;
            break;
        case SEEK_CUR:
            m_offset += offset;
            break;
        case SEEK_END:
            m_offset = m_file->size() + offset;
            break;
        default:
            break;
    }
}

}