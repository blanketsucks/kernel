#include <kernel/fs/file.h>
#include <kernel/fs/inode.h>

namespace kernel::fs {

size_t InodeFile::read(void* buffer, size_t size, size_t offset) {
    return m_inode.read(buffer, size, offset);
}

size_t InodeFile::write(const void* buffer, size_t size, size_t offset) {
    return m_inode.write(buffer, size, offset);
}

size_t InodeFile::size() const {
    return m_inode.size();
}

}