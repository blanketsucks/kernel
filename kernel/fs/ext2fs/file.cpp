#include <kernel/fs/ext2fs/file.h>

#include <kernel/fs/ext2fs/filesystem.h>
#include <kernel/fs/ext2fs/inode.h>

namespace kernel::ext2fs {

File::File(FileSystem* fs, InodeEntry* inode) : m_inode(inode), m_fs(fs) {}

size_t File::read(void* buffer, size_t size, size_t offset) {
    return m_inode->read(buffer, size, offset);
}

size_t File::write(const void*, size_t, size_t) {
    return 0; // TODO: Implement
}

}