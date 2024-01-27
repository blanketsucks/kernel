#include <kernel/fs/ext2fs/filesystem.h>
#include <kernel/posix/sys/stat.h>
#include <kernel/serial.h>

#include <std/bitmap.h>

namespace kernel::ext2fs {

BlockGroup::BlockGroup(FileSystem* fs, u32 index) : m_index(index), m_fs(fs) {
    fs->read_block_group(index, &m_descriptor);
}

void BlockGroup::flush() {
    m_fs->write_block_group(m_index, &m_descriptor);
}

FileSystem* FileSystem::s_instance = nullptr;

FileSystem* FileSystem::create(devices::BlockDevice* disk) {
    if (s_instance) return s_instance;

    if (disk->block_size() != SECTOR_SIZE) {
        return nullptr;
    }

    auto* superblock = new Superblock;
    disk->read_blocks(superblock, 2, 2);

    if (superblock->magic != MAGIC) {
        delete superblock;
        return nullptr;
    }

    s_instance = new FileSystem(superblock, disk);
    s_instance->m_block_groups.resize(s_instance->block_group_count());

    return s_instance;
}

FileSystem* FileSystem::instance() {
    return s_instance;
}

FileSystem::FileSystem(Superblock* superblock, devices::BlockDevice* disk) : m_superblock(superblock), m_disk(disk) {}

void FileSystem::flush_superblock() const {
    m_disk->write_blocks(m_superblock, 2, 2);
}

void FileSystem::read_block(u32 block, u8* buffer) const {
    this->read_blocks(block, 1, buffer);
}

void FileSystem::read_blocks(u32 block, u32 count, u8* buffer) const {
    u32 sector = block * (this->block_size() / SECTOR_SIZE);
    u32 sector_count = count * (this->block_size() / SECTOR_SIZE);

    m_disk->read_blocks(buffer, sector_count, sector);
}

void FileSystem::write_block(u32 block, const u8* buffer) const {
    this->write_blocks(block, 1, buffer);
}

void FileSystem::write_blocks(u32 block, u32 count, const u8* buffer) const {
    u32 sector = block * (this->block_size() / SECTOR_SIZE);
    u32 sector_count = count * (this->block_size() / SECTOR_SIZE);

    m_disk->write_blocks(buffer, sector_count, sector);
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

BlockGroup* FileSystem::get_block_group(u32 index) {
    if (index >= this->block_group_count()) {
        return nullptr;
    }

    BlockGroup* block_group = m_block_groups[index];
    if (!block_group) {
        block_group = new BlockGroup(this, index);
        m_block_groups[index] = block_group;
    }

    return block_group;
}

BlockGroup* FileSystem::find_block_group(const Function<IterationAction(BlockGroup*)>& predicate) {
    for (auto& group : m_block_groups) {
        if (predicate(group) == IterationAction::Break) {
            return group;
        }
    }

    return nullptr;
}

RefPtr<fs::Inode> FileSystem::inode(ino_t inode) {
    // auto iterator = m_inodes.find(inode);
    // if (iterator != m_inodes.end()) {
    //     return iterator->value;
    // }

    u32 block_group = (inode - 1) / m_superblock->inodes_per_group;
    u32 index = (inode - 1) % m_superblock->inodes_per_group;

    BlockGroup* group = this->get_block_group(block_group);
    if (!group) return nullptr;

    u32 block = group->inode_table() + (index) * sizeof(ext2fs::Inode) / this->block_size();
    u8 buffer[this->block_size()];

    this->read_block(block, buffer);
    ext2fs::Inode result = {};

    u32 offset = (index * sizeof(ext2fs::Inode)) % this->block_size();
    std::memcpy(&result, buffer + offset, sizeof(ext2fs::Inode));

    auto entry = new InodeEntry(this, result, inode);
    m_inodes.set(inode, entry);

    return entry;
}

ino_t FileSystem::root() const { 
    return ROOT_INODE;
}

RefPtr<InodeEntry> FileSystem::create_inode(mode_t mode, uid_t uid, gid_t gid) {
    BlockGroup* block_group = this->find_block_group([](BlockGroup* group) {
        return group->free_inodes() > 0 ? IterationAction::Break : IterationAction::Continue;
    });

    if (!block_group) {
        return nullptr;
    }
    
    // TODO: Move the below piece of code to BlockGroup::allocate_inode().
    auto& descriptor = block_group->descriptor();

    u8 buffer[this->block_size()];
    this->read_block(block_group->inode_bitmap(), buffer);

    std::Bitmap bitmap(buffer, this->superblock()->inodes_per_group);
    ino_t inode = bitmap.find_first_unset();

    bitmap.set(inode, true);
    this->write_block(block_group->inode_bitmap(), buffer);

    descriptor.free_inodes--;
    if (S_ISDIR(mode)) {
        descriptor.dir_count++;
    }
    
    block_group->flush();
    inode += 1 + block_group->index() * m_superblock->inodes_per_group;

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

ErrorOr<Vector<u32>> FileSystem::allocate_blocks(u32 count) {
    BlockGroup* block_group = this->find_block_group([count](BlockGroup* group) {
        return group->free_block_count() >= count ? IterationAction::Break : IterationAction::Continue;
    });

    // We found a block group that can hold the requested amount of blocks.
    if (block_group) {
        return block_group->allocate_blocks(count);
    }

    // If we didn't find one, we try to allocate from different block groups.
    Vector<u32> blocks;
    blocks.reserve(count);

    while (count) {
        block_group = this->find_block_group([](BlockGroup* group) {
            return group->free_block_count() > 0 ? IterationAction::Break : IterationAction::Continue;
        });

        if (!block_group) {
            return Error(ENOSPC);
        }

        Vector<u32> allocated = TRY(block_group->allocate_blocks(count));
        blocks.extend(allocated);

        count -= allocated.size();
    }

    return blocks;
}

ErrorOr<u32> FileSystem::allocate_block() {
    Vector<u32> blocks = TRY(this->allocate_blocks(1));
    return blocks[0];
}

void FileSystem::free_block(u32 block) {
    u32 block_group_index = (block - 1) / m_superblock->blocks_per_group;
    BlockGroup* block_group = this->get_block_group(block_group_index);

    if (!block_group) {
        return;
    }

    block_group->free_block(block);
}

}