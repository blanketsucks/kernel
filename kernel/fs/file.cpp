#include <kernel/fs/file.h>
#include <kernel/fs/inode.h>
#include <kernel/process/process.h>

namespace kernel::fs {

struct stat InodeFile::stat() const {
    return m_inode->stat();
}

ssize_t InodeFile::read(void* buffer, size_t size, size_t offset) {
    return m_inode->read(buffer, size, offset);
}

ssize_t InodeFile::write(const void* buffer, size_t size, size_t offset) {
    return m_inode->write(buffer, size, offset);
}

size_t InodeFile::size() const {
    return m_inode->size();
}

ErrorOr<void*> InodeFile::mmap(Process& process, size_t size, int) {
    auto* region = process.region_allocator().create_file_backed_region(this, size);
    if (!region) {
        return nullptr;
    }

    return reinterpret_cast<void*>(region->base());
}

}