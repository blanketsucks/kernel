#include <kernel/fs/ramfs/filesystem.h>
#include <kernel/posix/sys/stat.h>

namespace kernel::ramfs {

RefPtr<Inode> Inode::create(FileSystem* fs, String name, mode_t mode, dev_t dev, ino_t parent) {
    return RefPtr(new Inode(fs, fs->allocate_inode(), move(name), mode, dev, parent));
}

ErrorOr<size_t> Inode::read(void* buffer, size_t size, size_t offset) const {
    if (offset >= m_size) {
        return 0;
    } else if (offset + size > m_size) {
        size = m_size - offset;
    }

    memcpy(buffer, reinterpret_cast<u8*>(m_data) + offset, size);
    return size;
}

ErrorOr<size_t> Inode::write(const void* buffer, size_t size, size_t offset) {
    if (offset + size > m_size) {
        TRY(this->truncate(offset + size));
    }

    memcpy(reinterpret_cast<u8*>(m_data) + offset, buffer, size);
    return size;
}

ErrorOr<void> Inode::truncate(size_t size) {
    if (size == m_size) {
        return {};
    }

    void* data = kmalloc(size);
    if (!data) {
        return Error(ENOSPC);
    }

    memcpy(data, m_data, size);

    kfree(m_data);

    m_data = data;
    m_size = size;

    return {};
}

struct stat Inode::stat() const {
    struct stat stat = {};
    return stat;
}

void Inode::readdir(std::Function<IterationAction(const fs::DirectoryEntry&)> callback) const {
    if (!this->is_directory()) {
        return;
    }

    callback({ m_id, fs::DirectoryEntry::Directory, "." });
    callback({ m_parent, fs::DirectoryEntry::Directory, ".." });

    for (auto& [name, inode] : m_children) {
        auto type = inode->is_directory() ? fs::DirectoryEntry::Directory : fs::DirectoryEntry::RegularFile;
        callback({ inode->id(), type, name });
    }
}

ErrorOr<RefPtr<fs::Inode>> Inode::lookup(StringView name) const {
    auto it = m_children.find(name);
    if (it == m_children.end()) {
        return Error(ENOENT);
    }

    return { it->value };
}

ErrorOr<void> Inode::add_entry(String name, RefPtr<fs::Inode> inode) {
    if (!this->is_directory()) {
        return Error(ENOTDIR);
    } else if (m_children.contains(name)) {
        return Error(EEXIST);
    }

    m_children.set(name, inode);
    return {};
}

ErrorOr<void> Inode::remove_entry(StringView name) {
    if (!this->is_directory()) {
        return Error(ENOTDIR);
    }

    auto iterator = m_children.find(name);
    if (iterator == m_children.end()) {
        return Error(ENOENT);
    }

    m_children.remove(iterator);
    return {};
}

// TODO: Handle mode (correctly), uid, gid
ErrorOr<RefPtr<fs::Inode>> Inode::create_entry(String name, mode_t mode, dev_t dev, uid_t, gid_t) {
    if (!is_directory()) {
        return Error(ENOTDIR);
    }

    auto inode = Inode::create(m_fs, name, mode, dev, m_id);
    m_children.set(name, inode);

    return { inode };
}

ErrorOr<void> Inode::flush() {
    return {};
}

FileSystem::FileSystem() {
    m_root = Inode::create(this, "/", S_IFDIR | 0755, 0, 0);
}

ErrorOr<RefPtr<fs::Inode>> FileSystem::inode(ino_t id) {
    if (id == this->root()) {
        return { m_root };
    }

    return Error(ENOENT);
}

}
