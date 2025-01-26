#include <kernel/fs/ext2fs/inode.h>
#include <kernel/fs/ext2fs/filesystem.h>
#include <kernel/fs/ext2fs/ext2.h>

#include <kernel/devices/device.h>
#include <kernel/serial.h>

#include <std/format.h>

#define VERIFY_BLOCK(block)                         \
    if (!block) {                                   \
        auto result = m_fs->allocate_block();       \
        if (result.is_err()) {                      \
            return Error(ENOSPC);                   \
        }                                           \
        block = result.value();                     \
    }

namespace kernel::ext2fs {

InodeEntry::InodeEntry(FileSystem* fs, ext2fs::Inode inode, ino_t id) : Inode(id), m_inode(inode), m_fs(fs) {
    if (this->is_device()) {
        u32 device = m_inode.block_pointers[0];
        if (!device) {
            device = m_inode.block_pointers[1];
        }

        auto id = Device::decode(device);

        m_device_major = id.major;
        m_device_minor = id.minor;
    } else {
        this->read_block_pointers();
        m_entries = this->read_directory_entries();
    }
}

size_t InodeEntry::block_count() const {
    return (this->size() + m_fs->block_size() - 1) / m_fs->block_size();
}

u32 InodeEntry::block_group_index() const {
    return (m_id - 1) / m_fs->superblock()->inodes_per_group;
}

u32 InodeEntry::block_group_offset() const {
    return (m_id - 1) % m_fs->superblock()->inodes_per_group;
}

ssize_t InodeEntry::read(void* buffer, size_t size, size_t offset) const {
    if (offset >= this->size()) {
        return 0;
    } else if (offset + size > this->size()) {
        size = this->size() - offset;
    }

    size_t block_size = m_fs->block_size();

    size_t block = offset / block_size;
    size_t block_offset = offset % block_size;

    // size_t total_blocks = size / block_size;
    // size_t remaining_bytes = size % block_size;

    // FIXME: There is something wrong with the following codeblock and I don't know what it is.
    // if (total_blocks > 1) {
    //     u8* buf = reinterpret_cast<u8*>(buffer);
    //     u8 block_buffer[block_size];
    
    //     if (block_offset) {
    //         this->read_blocks(block, 1, block_buffer);
    //         memcpy(buf, block_buffer + block_offset, block_size - block_offset);

    //         buf += block_size - block_offset;

    //         block++;
    //         total_blocks--;
    //     }

    //     this->read_blocks(block, total_blocks, buf);
    //     if (remaining_bytes) {
    //         this->read_blocks(block + total_blocks, 1, block_buffer);
    //         memcpy(buf + size - remaining_bytes, block_buffer, remaining_bytes);
    //     }
        
    //     return size;
    // }

    size_t bytes_read = 0;
    u8 block_buffer[block_size];
 
    while (bytes_read < size) {
        this->read_blocks(block, 1, block_buffer);

        size_t bytes_to_read = std::min(size - bytes_read, block_size - block_offset);
        memcpy(reinterpret_cast<u8*>(buffer) + bytes_read, block_buffer + block_offset, bytes_to_read);

        bytes_read += bytes_to_read;
        block_offset = 0;

        block++;
    }

    return bytes_read;
}

ssize_t InodeEntry::write(const void* buffer, size_t size, size_t offset) {
    if (!size) {
        return 0;
    } else if (offset + size > this->size()) {
        this->truncate(offset + size);
    }

    u32 block_size = m_fs->block_size();

    size_t block = offset / block_size;
    size_t block_offset = offset % block_size;

    size_t bytes_written = 0;
    u8 block_buffer[block_size];

    while (bytes_written < size) {
        this->read_blocks(block, 1, block_buffer);

        size_t bytes_to_write = std::min(size - bytes_written, block_size - block_offset);
        memcpy(block_buffer + block_offset, reinterpret_cast<const u8*>(buffer) + bytes_written, bytes_to_write);

        this->write_blocks(block, 1, block_buffer);

        bytes_written += bytes_to_write;
        block_offset = 0;

        block++;
    }

    return 0;
}

void InodeEntry::truncate(size_t size) {
    if (this->size() == size) {
        return;
    }

    u32 block_size = m_fs->block_size();
    size_t block_count = (size + block_size - 1) / block_size;

    if (block_count > this->block_count()) {
        auto result = m_fs->allocate_blocks(block_count - this->block_count());
        if (result.is_err()) {
            return;
        }

        for (u32 block : result.value()) {
            m_block_pointers.append(block);
        }

        m_inode.size_lower = size;
    } else {
        // TODO: Implement case where we need to shrink the inode.
    }

    this->flush();
}

struct stat InodeEntry::stat() const {
    struct stat stat = {};

    stat.st_ino = m_id;
    stat.st_mode = m_inode.mode;
    stat.st_nlink = m_inode.hard_link_count;
    stat.st_uid = m_inode.user_id;
    stat.st_gid = m_inode.group_id;
    stat.st_size = this->size();
    stat.st_blksize = m_fs->block_size();
    stat.st_blocks = this->size() / 512;

    stat.st_atim.tv_sec = m_inode.last_access_time;
    stat.st_mtim.tv_sec = m_inode.last_modification_time;
    stat.st_ctim.tv_sec = m_inode.creation_time;

    return stat;
}

void InodeEntry::read_blocks(size_t block, size_t count, u8* buffer) const {
    if (count == 1) {
        u32 block_pointer = this->get_block_pointer(block);
        if (block_pointer) {
            m_fs->read_block(block_pointer, buffer);
        }

        return;
    }
    
    bool is_contiguous = false;

    u32 start_block = 0;
    size_t contiguous_count = 0;

    size_t max_contiguous_blocks = m_fs->max_io_block_count();
    for (size_t i = 0; i < count; i++) {
        i32 block_pointer = this->get_block_pointer(block + i);
        if (!block_pointer) {
            continue;
        }

        if (!is_contiguous) {
            is_contiguous = true;
            start_block = block_pointer;

            contiguous_count = 1;
            continue;
        }

        bool is_next_block_contiguous = static_cast<u32>(block_pointer) == (start_block + contiguous_count);
        if (is_next_block_contiguous && contiguous_count < max_contiguous_blocks) {
            contiguous_count++;
            continue;
        }

        m_fs->read_blocks(start_block, contiguous_count, buffer);
        buffer += m_fs->block_size() * contiguous_count;

        is_contiguous = true;
        start_block = block_pointer;

        contiguous_count = 1;
    }

    if (is_contiguous) {
        m_fs->read_blocks(start_block, contiguous_count, buffer);
    }

    // for (size_t i = 0; i < count; i++) {
    //     i32 block_pointer = this->get_block_pointer(block + i);
    //     if (block_pointer == 0) {
    //         continue;
    //     }

    //     m_fs->read_block(block_pointer, buffer);
    //     buffer += m_fs->block_size();
    // }
}

void InodeEntry::write_blocks(size_t block, size_t count, const u8* buffer) {
    for (size_t i = 0; i < count; i++) {
        i32 block_pointer = this->get_block_pointer(block + i);
        if (block_pointer == 0) {
            continue;
        }

        m_fs->write_block(block_pointer, buffer);
        buffer += m_fs->block_size();
    }
}

Vector<fs::DirectoryEntry> InodeEntry::read_directory_entries() const {
    if (!this->is_directory()) return {};

    u8 buffer[m_fs->block_size()];
    Vector<fs::DirectoryEntry> entries;

    for (size_t i = 0; i < this->block_count(); i++) {
        memset(buffer, 0, m_fs->block_size());
        this->read_blocks(i, 1, buffer);

        size_t offset = 0;
        while (offset < m_fs->block_size()) {
            DirEntry* entry = reinterpret_cast<DirEntry*>(buffer + offset);
            StringView name = StringView(
                reinterpret_cast<const char*>(buffer + offset + sizeof(DirEntry)), 
                entry->name_length
            );

            if (entry->inode == 0) break;
            offset += entry->size;

            entries.append(fs::DirectoryEntry(entry->inode, static_cast<fs::DirectoryEntry::Type>(entry->type_indicator), String(name)));
        }
    }

    return entries;
}

void InodeEntry::write_directory_entries() {
    if (!this->is_directory()) return;

    ASSERT(false, "Writing directory entries is not implemented yet.");
}

ErrorOr<void> InodeEntry::add_directory_entry(ino_t inode, String name, fs::DirectoryEntry::Type type) {
    if (!this->is_directory()) return Error(ENOTDIR);
    if (name.size() > MAX_NAME_SIZE) return Error(ENAMETOOLONG);

    for (auto& entry : m_entries) {
        if (entry.name == name) {
            return Error(EEXIST);
        }
    }

    m_entries.append({ inode, type, move(name) });
    return {};
}

RefPtr<fs::Inode> InodeEntry::lookup(StringView name) const {
    if (!this->is_directory()) {
        return nullptr;
    }

    for (auto& entry : m_entries) {
        if (entry.name == name) {
            return m_fs->inode(entry.inode);
        }
    }

    return nullptr;
}

u32 InodeEntry::get_block_pointer(size_t index) const {
    if (index >= m_block_pointers.size()) {
        return 0;
    }

    return m_block_pointers[index];
}

void InodeEntry::read_block_pointers() {
    for (u32 i = 0; i < 12; i++) {
        u32 block_pointer = m_inode.block_pointers[i];
        if (!block_pointer) {
            continue;
        }

        m_block_pointers.append(block_pointer);
    }

#define READ_BLOCK_POINTERS(m) if (m_inode.m) { this->read_##m##s(m_inode.m); }

    READ_BLOCK_POINTERS(singly_indirect_block_pointer);
    READ_BLOCK_POINTERS(doubly_indirect_block_pointer);
    READ_BLOCK_POINTERS(triply_indirect_block_pointer);

#undef READ_BLOCK_POINTER
}

void InodeEntry::read_singly_indirect_block_pointers(u32 block) {
    u8 buffer[m_fs->block_size()];
    m_fs->read_block(block, buffer);

    m_indirect_block_pointers.append(block);

    u32* pointers = reinterpret_cast<u32*>(buffer);
    for (size_t i = 0; i < m_fs->block_size() / sizeof(u32); i++) {
        if (!pointers[i]) {
            continue;
        }

        m_block_pointers.append(pointers[i]);
    }
}

void InodeEntry::read_doubly_indirect_block_pointers(u32 block) {
    u8 buffer[m_fs->block_size()];
    m_fs->read_block(block, buffer);

    m_indirect_block_pointers.append(block);

    u32* pointers = reinterpret_cast<u32*>(buffer);
    for (size_t i = 0; i < m_fs->block_size() / sizeof(u32); i++) {
        if (!pointers[i]) {
            continue;
        }

        this->read_singly_indirect_block_pointers(pointers[i]);
    }
}

void InodeEntry::read_triply_indirect_block_pointers(u32 block) {
    u8 buffer[m_fs->block_size()];
    m_fs->read_block(block, buffer);

    m_indirect_block_pointers.append(block);

    u32* pointers = reinterpret_cast<u32*>(buffer);
    for (size_t i = 0; i < m_fs->block_size() / sizeof(u32); i++) {
        if (!pointers[i]) {
            continue;
        }

        this->read_doubly_indirect_block_pointers(pointers[i]);
    }
}

ErrorOr<void> InodeEntry::write_block_pointers() {
    if (this->is_device()) {
        u32 device = Device::encode(m_device_major, m_device_minor);
        m_inode.block_pointers[0] = device;

        return {};
    }

    m_indirect_block_pointers.clear();

    u32 block_size = m_fs->block_size();
    u32 block_pointers_per_block = block_size / sizeof(u32);

    // Write the first 12 block pointers directly to the inodes.
    // The rest of the block pointers will be in the form of indirect blocks.
    for (u8 i = 0; i < 12; i++) {
        m_inode.block_pointers[i] = this->get_block_pointer(i);
    }

    if (this->block_count() <= 12) {
        this->set_disk_sectors();
        m_inode.singly_indirect_block_pointer = 0;

        return {};
    }

#define WRITE_BLOCK_POINTER(m)                        \
    VERIFY_BLOCK(m_inode.m);                          \
    TRY(this->write_##m##s(m_inode.m));


    WRITE_BLOCK_POINTER(singly_indirect_block_pointer);
    if (this->block_count() <= 12 + block_pointers_per_block) {
        this->set_disk_sectors();
        m_inode.doubly_indirect_block_pointer = 0;

        return {};
    }

    WRITE_BLOCK_POINTER(doubly_indirect_block_pointer);
    if (this->block_count() <= 12 + block_pointers_per_block * block_pointers_per_block) {
        this->set_disk_sectors();
        m_inode.triply_indirect_block_pointer = 0;

        return {};
    }

    WRITE_BLOCK_POINTER(triply_indirect_block_pointer);
    this->set_disk_sectors();

    return {};

#undef WRITE_BLOCK_POINTER
}



ErrorOr<void> InodeEntry::write_singly_indirect_block_pointers(u32 block) {
    u32 block_size = m_fs->block_size();

    u8 buffer[block_size];
    m_fs->read_block(block, buffer);

    m_indirect_block_pointers.append(block);

    u32* singly_indirect_pointers = reinterpret_cast<u32*>(buffer);
    for (size_t i = 0; i < block_size / sizeof(u32); i++) {
        singly_indirect_pointers[i] = this->get_block_pointer(i + 12);
    }

    m_fs->write_block(block, buffer);
    return {};
}

ErrorOr<void> InodeEntry::write_doubly_indirect_block_pointers(u32 block) {
    u32 block_size = m_fs->block_size();

    u8 buffer[block_size];
    m_fs->read_block(block, buffer);

    m_indirect_block_pointers.append(block);

    u32* doubly_indirect_pointers = reinterpret_cast<u32*>(buffer);
    for (size_t i = 0; i < block_size / sizeof(u32); i++) {
        VERIFY_BLOCK(doubly_indirect_pointers[i]);
        this->write_singly_indirect_block_pointers(doubly_indirect_pointers[i]);
    }

    m_fs->write_block(block, buffer);
    return {};
}

ErrorOr<void> InodeEntry::write_triply_indirect_block_pointers(u32) {
    ASSERT(false, "Writing triply indirect block pointers is not implemented yet.");
    return {};
}

void InodeEntry::set_disk_sectors() {
    u32 block_size = m_fs->block_size();
    m_inode.disk_sectors = (m_block_pointers.size() + m_indirect_block_pointers.size()) * block_size / SECTOR_SIZE;
}

ErrorOr<void> InodeEntry::add_entry(String name, RefPtr<fs::Inode> inode) {
    mode_t mode = inode->mode();
    fs::DirectoryEntry::Type type = fs::DirectoryEntry::Unknown;

    switch (mode & S_IFMT) {
        case S_IFDIR:
            this->m_inode.hard_link_count++;
            type = fs::DirectoryEntry::Directory; break;
        case S_IFREG:
            type = fs::DirectoryEntry::RegularFile; break;
        case S_IFLNK:
            type = fs::DirectoryEntry::SymbolicLink; break;
        case S_IFSOCK:
            type = fs::DirectoryEntry::Socket; break;
        case S_IFIFO:
            type = fs::DirectoryEntry::FIFO; break;
        case S_IFCHR:
            type = fs::DirectoryEntry::CharacterDevice; break;
        case S_IFBLK:
            type = fs::DirectoryEntry::BlockDevice; break;
        default:
            return Error(EINVAL);
    }

    return this->add_directory_entry(inode->id(), name, type);
}

RefPtr<fs::Inode> InodeEntry::create_entry(String name, mode_t mode, uid_t uid, gid_t gid) {
    auto inode = m_fs->create_inode(mode, uid, gid);
    if (!inode) {
        return nullptr;
    }

    this->add_entry(name, inode);
    return inode;
}

void InodeEntry::flush() {
    this->write_block_pointers();

    u32 block_size = m_fs->block_size();
    auto superblock = m_fs->superblock();

    u32 block_group = (m_id - 1) / superblock->inodes_per_group;
    u32 index = (m_id - 1) % superblock->inodes_per_group;

    BlockGroup* group = m_fs->get_block_group(block_group);
    if (!group) {
        return;
    }

    u32 block = group->inode_table() + index * sizeof(ext2fs::Inode) / block_size;
    u8 buffer[block_size];

    m_fs->read_block(block, buffer);

    u32 offset = (index * sizeof(ext2fs::Inode)) % block_size;
    memcpy(buffer + offset, &m_inode, sizeof(ext2fs::Inode));

    m_fs->write_block(block, buffer);
}

}