#if 0

#include <kernel/common.h>

#include <kernel/fs/ramfs/filesystem.h>
#include <kernel/fs/ramfs/file.h>

namespace kernel::ramfs {

File::File(FileSystem* fs, Node* node) : m_node(node), m_fs(fs) {}

size_t File::read(void* buffer, size_t size, size_t offset) {
    return m_fs->read(m_node, buffer, size, offset).value();
}

size_t File::write(const void* buffer, size_t size, size_t offset) {
    return m_fs->write(m_node, buffer, size, offset).value();
}

}

#endif