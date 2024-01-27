#pragma once

#include <kernel/common.h>
#include <kernel/fs/ext2fs/ext2.h>

#include <std/vector.h>

namespace kernel::ext2fs {

class FileSystem;

class BlockGroup {
public:
    BlockGroup(FileSystem* fs, u32 index);

    u32 index() const { return m_index; }

    BlockGroupDescriptor& descriptor() { return m_descriptor; }
    BlockGroupDescriptor const& descriptor() const { return m_descriptor; }

    u32 block_bitmap() const { return m_descriptor.block_bitmap; }
    u32 inode_bitmap() const { return m_descriptor.inode_bitmap; }
    u32 inode_table() const { return m_descriptor.inode_table; }
    u16 free_block_count() const { return m_descriptor.free_blocks; }
    u16 free_inodes() const { return m_descriptor.free_inodes; }
    u16 directory_count() const { return m_descriptor.dir_count; }

    void flush();

    ErrorOr<Vector<u32>> allocate_blocks(u32 count);
    u32 allocate_block();

    void free_blocks(const Vector<u32>& blocks);
    void free_block(u32 block);

private:
    BlockGroupDescriptor m_descriptor;
    u32 m_index;

    FileSystem* m_fs;
};

}