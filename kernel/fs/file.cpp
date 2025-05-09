#include <kernel/fs/file.h>
#include <kernel/fs/inode.h>
#include <kernel/process/process.h>

#include <std/format.h>

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
    auto* region = process.allocate_file_backed_region(this, size);
    if (!region) {
        return nullptr;
    }

    return region;
}

ssize_t InodeFile::readdir(void* buf, size_t size) {
    if (!m_inode->is_directory()) {
        return -ENOTDIR;
    }

    size_t offset = 0;
    u8* buffer = reinterpret_cast<u8*>(buf);

    bool no_space = false;

    m_inode->readdir([&](const DirectoryEntry& entry) {
        size_t length = sizeof(ino_t) + sizeof(u8) + entry.name.size() + 1;
        if (offset + length > size) {
            no_space = true;
            return IterationAction::Break;
        }

        // TODO: Figure out a nicer way to do this
        memcpy(buffer + offset, &entry.inode, sizeof(ino_t));
        offset += sizeof(ino_t);

        memcpy(buffer + offset, &entry.type, sizeof(u8));
        offset += sizeof(u8);

        size_t name_length = entry.name.size();

        memcpy(buffer + offset, &name_length, sizeof(size_t));
        offset += sizeof(size_t);
    
        memcpy(buffer + offset, entry.name.data(), entry.name.size());
        offset += entry.name.size();

        return IterationAction::Continue;
    });

    if (no_space) {
        return -EINVAL;
    }

    return offset;
}

}