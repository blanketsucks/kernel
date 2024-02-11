#include <kernel/fs/ext2fs/inode.h>
#include <kernel/fs/ext2fs/filesystem.h>
#include <kernel/fs/ext2fs/ext2.h>

#include <kernel/devices/device.h>
#include <kernel/serial.h>

#define VERIFY_BLOCK(block)                         \
    if (!block) {                                   \
        auto result = ext2fs()->allocate_block();   \
        if (result.is_err()) {                      \
            return Error(ENOSPC);                   \
        }                                           \
        block = result.value();                     \
    }

using kernel::devices::Device;

namespace kernel::ext2fs {

InodeEntry::InodeEntry(FileSystem* fs, ext2fs::Inode inode, ino_t id) : Inode(fs, id), m_inode(inode) {
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

inline FileSystem* InodeEntry::ext2fs() const {
    return static_cast<FileSystem*>(m_fs);
}

size_t InodeEntry::block_count() const {
    return (this->size() + ext2fs()->block_size() - 1) / ext2fs()->block_size();
}

u32 InodeEntry::block_group_index() const {
    return (m_id - 1) / ext2fs()->superblock()->inodes_per_group;
}

u32 InodeEntry::block_group_offset() const {
    return (m_id - 1) % ext2fs()->superblock()->inodes_per_group;
}

size_t InodeEntry::read(void* buffer, size_t size, size_t offset) const {
    if (offset >= this->size()) {
        return 0;
    } else if (offset + size > this->size()) {
        size = this->size() - offset;
    }

    size_t block = offset / ext2fs()->block_size();
    size_t block_offset = offset % ext2fs()->block_size();

    size_t bytes_read = 0;
    u8 block_buffer[ext2fs()->block_size()];
 
    while (bytes_read < size) {
        this->read_blocks(block, 1, block_buffer);

        size_t bytes_to_read = min(size - bytes_read, ext2fs()->block_size() - block_offset);
        std::memcpy(reinterpret_cast<u8*>(buffer) + bytes_read, block_buffer + block_offset, bytes_to_read);

        bytes_read += bytes_to_read;
        block_offset = 0;

        block++;
    }

    return bytes_read;
}

size_t InodeEntry::write(const void* buffer, size_t size, size_t offset) {
    if (!size) {
        return 0;
    } else if (offset + size > this->size()) {
        this->truncate(offset + size);
    }

    u32 block_size = ext2fs()->block_size();

    size_t block = offset / block_size;
    size_t block_offset = offset % block_size;

    size_t bytes_written = 0;
    u8 block_buffer[block_size];

    while (bytes_written < size) {
        this->read_blocks(block, 1, block_buffer);

        size_t bytes_to_write = min(size - bytes_written, block_size - block_offset);
        std::memcpy(block_buffer + block_offset, reinterpret_cast<const u8*>(buffer) + bytes_written, bytes_to_write);

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

    u32 block_size = ext2fs()->block_size();
    size_t block_count = (size + block_size - 1) / block_size;

    if (block_count > this->block_count()) {
        auto result = ext2fs()->allocate_blocks(block_count - this->block_count());
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
    stat.st_blksize = ext2fs()->block_size();
    stat.st_blocks = this->size() / 512;

    stat.st_atim.tv_sec = m_inode.last_access_time;
    stat.st_mtim.tv_sec = m_inode.last_modification_time;
    stat.st_ctim.tv_sec = m_inode.creation_time;

    return stat;
}

void InodeEntry::read_blocks(size_t block, size_t count, u8* buffer) const {
    for (size_t i = 0; i < count; i++) {
        i32 block_pointer = this->get_block_pointer(block + i);
        if (block_pointer == 0) {
            continue;
        }

        ext2fs()->read_block(block_pointer, buffer);
        buffer += ext2fs()->block_size();
    }
}

void InodeEntry::write_blocks(size_t block, size_t count, const u8* buffer) {
    for (size_t i = 0; i < count; i++) {
        i32 block_pointer = this->get_block_pointer(block + i);
        if (block_pointer == 0) {
            continue;
        }

        ext2fs()->write_block(block_pointer, buffer);
        buffer += ext2fs()->block_size();
    }
}

Vector<fs::DirectoryEntry> InodeEntry::read_directory_entries() const {
    if (!this->is_directory()) return {};

    u8 buffer[ext2fs()->block_size()];
    Vector<fs::DirectoryEntry> entries;

    for (size_t i = 0; i < this->block_count(); i++) {
        this->read_blocks(i, 1, buffer);

        size_t offset = 0;
        while (offset < ext2fs()->block_size()) {
            DirEntry* entry = reinterpret_cast<DirEntry*>(buffer + offset);
            StringView name = StringView(
                reinterpret_cast<const char*>(buffer + offset + sizeof(DirEntry)), 
                entry->name_length
            );

            if (entry->inode == 0) break;
            offset += entry->size;

            entries.append(
                { entry->inode, static_cast<fs::DirectoryEntry::Type>(entry->type_indicator), name }
            );
        }
    }

    return entries;
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
    if (!this->is_directory()) return nullptr;

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
    for (i32 block_pointer : m_inode.block_pointers) {
        if (!block_pointer) continue;
        m_block_pointers.append(block_pointer);
    }

#define READ_BLOCK_POINTERS(m) if (m_inode.m) { this->read_##m##s(m_inode.m); }

    READ_BLOCK_POINTERS(singly_indirect_block_pointer);
    READ_BLOCK_POINTERS(doubly_indirect_block_pointer);
    READ_BLOCK_POINTERS(triply_indirect_block_pointer);

#undef READ_BLOCK_POINTER
}

void InodeEntry::read_singly_indirect_block_pointers(u32 block) {
    u8 buffer[ext2fs()->block_size()];
    ext2fs()->read_block(block, buffer);

    m_indirect_block_pointers.append(block);

    u32* pointers = reinterpret_cast<u32*>(buffer);
    for (size_t i = 0; i < ext2fs()->block_size() / sizeof(u32); i++) {
        m_block_pointers.append(pointers[i]);
    }
}

void InodeEntry::read_doubly_indirect_block_pointers(u32 block) {
    u8 buffer[ext2fs()->block_size()];
    ext2fs()->read_block(block, buffer);

    m_indirect_block_pointers.append(block);

    u32* pointers = reinterpret_cast<u32*>(buffer);
    for (size_t i = 0; i < ext2fs()->block_size() / sizeof(u32); i++) {
        if (!buffer[i]) continue;
        this->read_singly_indirect_block_pointers(pointers[i]);
    }
}

void InodeEntry::read_triply_indirect_block_pointers(u32 block) {
    u8 buffer[ext2fs()->block_size()];
    ext2fs()->read_block(block, buffer);

    m_indirect_block_pointers.append(block);

    u32* pointers = reinterpret_cast<u32*>(buffer);
    for (size_t i = 0; i < ext2fs()->block_size() / sizeof(u32); i++) {
        if (!buffer[i]) continue;
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

    u32 block_size = ext2fs()->block_size();
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
    u32 block_size = ext2fs()->block_size();

    u8 buffer[block_size];
    ext2fs()->read_block(block, buffer);

    m_indirect_block_pointers.append(block);

    u32* singly_indirect_pointers = reinterpret_cast<u32*>(buffer);
    for (size_t i = 0; i < block_size / sizeof(u32); i++) {
        singly_indirect_pointers[i] = this->get_block_pointer(i + 12);
    }

    ext2fs()->write_block(block, buffer);
    return {};
}

ErrorOr<void> InodeEntry::write_doubly_indirect_block_pointers(u32 block) {
    u32 block_size = ext2fs()->block_size();

    u8 buffer[block_size];
    ext2fs()->read_block(block, buffer);

    m_indirect_block_pointers.append(block);

    u32* doubly_indirect_pointers = reinterpret_cast<u32*>(buffer);
    for (size_t i = 0; i < block_size / sizeof(u32); i++) {
        VERIFY_BLOCK(doubly_indirect_pointers[i]);
        this->write_singly_indirect_block_pointers(doubly_indirect_pointers[i]);
    }

    ext2fs()->write_block(block, buffer);
    return {};
}

ErrorOr<void> InodeEntry::write_triply_indirect_block_pointers(u32) {
    ASSERT(false, "Writing triply indirect block pointers is not implemented yet.");
    return {};
}

void InodeEntry::set_disk_sectors() {
    u32 block_size = ext2fs()->block_size();
    m_inode.disk_sectors = (m_block_pointers.size() + m_indirect_block_pointers.size()) * block_size / SECTOR_SIZE;
}

void InodeEntry::flush() {
    this->write_block_pointers();

    u32 block_size = ext2fs()->block_size();
    auto superblock = ext2fs()->superblock();

    u32 block_group = (m_id - 1) / superblock->inodes_per_group;
    u32 index = (m_id - 1) % superblock->inodes_per_group;

    BlockGroup* group = ext2fs()->get_block_group(block_group);
    if (!group) {
        return;
    }

    u32 block = group->inode_table() + index * sizeof(ext2fs::Inode) / block_size;
    u8 buffer[block_size];

    ext2fs()->read_block(block, buffer);

    u32 offset = (index * sizeof(ext2fs::Inode)) % block_size;
    std::memcpy(buffer + offset, &m_inode, sizeof(ext2fs::Inode));

    ext2fs()->write_block(block, buffer);
}

}