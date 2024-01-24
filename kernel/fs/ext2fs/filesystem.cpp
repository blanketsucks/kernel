#include <kernel/fs/ext2fs/filesystem.h>

#include <kernel/posix/sys/stat.h>
#include <std/bitmap.h>
#include <kernel/serial.h>

namespace kernel::ext2fs {

FileSystem* FileSystem::s_instance = nullptr;

FileSystem* FileSystem::create(devices::PATADevice* disk) {
    if (s_instance) return s_instance;

    auto* superblock = new Superblock;
    disk->read_sectors(2, 2, reinterpret_cast<u8*>(superblock));

    if (superblock->magic != MAGIC) {
        delete superblock;
        return nullptr;
    }

    s_instance = new FileSystem(superblock, disk);
    s_instance->m_block_group_descriptors.resize(s_instance->block_group_count());

    return s_instance;
}

FileSystem* FileSystem::instance() {
    return s_instance;
}

FileSystem::FileSystem(Superblock* superblock, devices::PATADevice* disk) : m_superblock(superblock), m_disk(disk) {}

void FileSystem::flush_superblock() const {
    m_disk->write_sectors(2, 2, reinterpret_cast<u8*>(m_superblock));
}

void FileSystem::read_block(u32 block, u8* buffer) const {
    this->read_blocks(block, 1, buffer);
}

void FileSystem::read_blocks(u32 block, u32 count, u8* buffer) const {
    u32 sector = block * (this->block_size() / SECTOR_SIZE);
    u32 sector_count = count * (this->block_size() / SECTOR_SIZE);

    m_disk->read_sectors(sector, sector_count, buffer);
}

void FileSystem::write_block(u32 block, const u8* buffer) const {
    this->write_blocks(block, 1, buffer);
}

void FileSystem::write_blocks(u32 block, u32 count, const u8* buffer) const {
    u32 sector = block * (this->block_size() / SECTOR_SIZE);
    u32 sector_count = count * (this->block_size() / SECTOR_SIZE);

    m_disk->write_sectors(sector, sector_count, buffer);
}

void FileSystem::read_block_group(u32 index, BlockGroupDescriptor* group) const {
    u32 block = (index * sizeof(BlockGroupDescriptor)) / this->block_size();
    if (this->block_size() == 1024) {
        // If the block size is 1024 bytes per block, the Block Group Descriptor Table will begin at block 2. 
        block += 2;
    } else {
        // For any other block size, it will begin at block 1.
        block += 1;
    }

    u8 buffer[this->block_size()];
    this->read_block(block, buffer);

    u32 offset = (index * sizeof(BlockGroupDescriptor)) % this->block_size();
    std::memcpy(group, buffer + offset, sizeof(BlockGroupDescriptor));
}

void FileSystem::write_block_group(u32 index, const BlockGroupDescriptor* group) const {
    u32 block = (index * sizeof(BlockGroupDescriptor)) / this->block_size();
    if (this->block_size() == 1024) {
        // If the block size is 1024 bytes per block, the Block Group Descriptor Table will begin at block 2. 
        block += 2;
    } else {
        // For any other block size, it will begin at block 1.
        block += 1;
    }

    u8 buffer[this->block_size()];
    this->read_block(block, buffer);

    u32 offset = (index * sizeof(BlockGroupDescriptor)) % this->block_size();
    std::memcpy(buffer + offset, group, sizeof(BlockGroupDescriptor));

    this->write_block(block, buffer);
}


BlockGroupDescriptor* FileSystem::get_block_group(u32 index) {
    if (index >= this->block_group_count()) {
        return nullptr;
    }

    BlockGroupDescriptor* group = m_block_group_descriptors[index];
    if (!group) {
        group = new BlockGroupDescriptor;
        this->read_block_group(index, group);

        m_block_group_descriptors[index] = group;
    }

    return m_block_group_descriptors[index];
}

RefPtr<fs::Inode> FileSystem::inode(ino_t inode) {
    auto iterator = m_inodes.find(inode);
    if (iterator != m_inodes.end()) {
        return iterator->value;
    }

    u32 block_group = (inode - 1) / m_superblock->inodes_per_group;
    u32 index = (inode - 1) % m_superblock->inodes_per_group;

    BlockGroupDescriptor* group = this->get_block_group(block_group);
    if (!group) return nullptr;

    u32 block = group->inode_table + (index) * sizeof(Inode) / this->block_size();
    u8 buffer[this->block_size()];

    this->read_block(block, buffer);
    Inode result = {};

    u32 offset = (index * sizeof(Inode)) % this->block_size();
    std::memcpy(&result, buffer + offset, sizeof(Inode));

    auto entry = new InodeEntry(this, result, inode);
    m_inodes.set(inode, entry);

    return entry;
}

ino_t FileSystem::root() const { 
    return ROOT_INODE;
}

RefPtr<InodeEntry> FileSystem::create_inode(mode_t mode, uid_t uid, gid_t gid) {
    BlockGroupDescriptor* block_group = nullptr;
    u32 block_group_index = 0;

    for (auto& group : m_block_group_descriptors) {
        if (group->free_inodes > 0) {
            block_group = group;
            break;
        }

        block_group_index++;
    }

    if (!block_group) return nullptr;

    u8 buffer[this->block_size()];
    this->read_block(block_group->inode_bitmap, buffer);

    std::Bitmap bitmap(buffer, this->block_size());
    ino_t inode = bitmap.find_first_unset();

    bitmap.set(inode, true);
    this->write_block(block_group->inode_bitmap, buffer);

    block_group->free_inodes--;
    if (S_ISDIR(mode)) {
        block_group->dir_count++;
    }
    
    this->write_block_group(block_group_index, block_group);
    inode += 1 + block_group_index * m_superblock->inodes_per_group;

    Inode result = {};

    result.mode = mode;
    result.user_id = uid;
    result.group_id = gid;
    result.size_lower = 0;
    result.hard_link_count = S_ISDIR(mode) ? 1 : 0;

    auto entry = RefPtr<InodeEntry>::make(this, result, inode);

    entry->flush();
    return entry;
}

}