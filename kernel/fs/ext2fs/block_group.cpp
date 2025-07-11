#include <kernel/fs/ext2fs/block_group.h>
#include <kernel/fs/ext2fs/filesystem.h>
#include <kernel/serial.h>

#include <std/bitmap.h>

namespace kernel::ext2fs {

BlockGroup::BlockGroup(FileSystem* fs, u32 index) : m_index(index), m_fs(fs) {
    fs->read_block_group(index, &m_descriptor);
}

void BlockGroup::flush() {
    m_fs->write_block_group(m_index, &m_descriptor);
}


ErrorOr<Vector<u32>> BlockGroup::allocate_blocks(u32 count) {
    if (this->free_block_count() < count) {
        return Error(ENOSPC);
    } else if (count == 0) {
        return Vector<u32>();
    }

    Vector<u32> blocks;
    blocks.reserve(count);

    auto& descriptor = this->descriptor();
    auto* superblock = m_fs->superblock();

    u32 first_block = superblock->blocks_per_group * m_index;
    if (m_fs->block_size() == 1024) {
        first_block += 1;
    }

    u8 buffer[m_fs->block_size()];
    m_fs->read_block(this->block_bitmap(), buffer);

    auto bitmap = std::Bitmap(buffer, superblock->blocks_per_group);
    for (u32 i = 0; i < superblock->blocks_per_group; i++) {
        if (bitmap.get(i)) {
            continue;
        }

        bitmap.set(i, true);
        blocks.append(first_block + i);

        if (blocks.size() == count) {
            break;
        }

        descriptor.free_blocks--;
        superblock->free_blocks--;
    }

    m_fs->write_block(this->block_bitmap(), buffer);
    this->flush();

    m_fs->flush_superblock();
    return blocks;
}

u32 BlockGroup::allocate_block() {
    auto result = this->allocate_blocks(1);
    if (result.is_err()) {
        return 0;
    }

    return result.value()[0];
}

void BlockGroup::free_blocks(const Vector<u32>& blocks) {
    u8 buffer[m_fs->block_size()];
    m_fs->read_block(this->block_bitmap(), buffer);

    auto& descriptor = this->descriptor();
    auto* superblock = m_fs->superblock();

    u32 first_block = superblock->blocks_per_group * m_index;

    auto bitmap = std::Bitmap(buffer, superblock->blocks_per_group);
    for (auto block : blocks) {
        bitmap.set(block - first_block, false);
    }

    m_fs->write_block(this->block_bitmap(), buffer);

    descriptor.free_blocks += blocks.size();
    superblock->free_blocks += blocks.size();

    this->flush();
    m_fs->flush_superblock();
}

void BlockGroup::free_block(u32 block) {
    this->free_blocks({ block });
}

u32 BlockGroup::allocate_inode(bool is_directory) {
    auto& descriptor = this->descriptor();
    auto& superblock = *m_fs->superblock();

    u8 buffer[m_fs->block_size()];
    m_fs->read_block(this->inode_bitmap(), buffer);

    std::Bitmap bitmap(buffer, superblock.inodes_per_group);
    ino_t inode = bitmap.find_first_unset();

    bitmap.set(inode, true);
    m_fs->write_block(this->inode_bitmap(), buffer);

    descriptor.free_inodes--;
    superblock.free_inodes--;

    if (is_directory) {
        descriptor.dir_count++;
    }
    
    this->flush();
    m_fs->flush_superblock();

    return inode;
}

}