#pragma once

#include <kernel/common.h>

#include <kernel/fs/ext2fs/ext2.h>
#include <kernel/fs/ext2fs/inode.h>
#include <kernel/fs/ext2fs/block_group.h>
#include <kernel/fs/filesystem.h>

#include <kernel/devices/block_device.h>

#include <std/function.h>
#include <std/memory.h>
#include <std/hash_map.h>

namespace kernel::ext2fs {

class FileSystem : public fs::FileSystem {
public:
    static FileSystem* create(BlockDevice* disk);

    RefPtr<fs::Inode> inode(ino_t inode) override;
    ino_t root() const override { return ROOT_INODE; }

    fs::FileSystemID id() const override { return fs::FileSystemID::Ext2FS; }
    StringView name() const override { return "ext2"; }

    size_t max_io_block_count() const {
        // Convert the disk->max_io_block_count() that is based on the disk block size to the filesystem block size
        int ratio = block_size() / m_disk->block_size();
        return m_disk->max_io_block_count() / ratio;
    }

    Superblock* superblock() { return m_superblock; }
    Superblock const* superblock() const { return m_superblock; }

    u32 block_size() const { return 1024 << m_superblock->block_size; }
    u32 block_group_count() const { return (m_superblock->total_blocks + m_superblock->blocks_per_group - 1) / m_superblock->blocks_per_group; }

    void flush_superblock() const;

    BlockGroup* get_block_group(u32 index);
    BlockGroup* find_block_group(
        const Function<IterationAction(BlockGroup*)>& predicate
    );

    void read_block_group(u32 index, BlockGroupDescriptor* group) const;
    void write_block_group(u32 index, const BlockGroupDescriptor* group) const;

    void read_block(u32 block, u8* buffer) const;
    void read_blocks(u32 block, u32 count, u8* buffer) const;

    void write_block(u32 block, const u8* buffer) const;
    void write_blocks(u32 block, u32 count, const u8* buffer) const;

    ErrorOr<Vector<u32>> allocate_blocks(u32 count);
    ErrorOr<u32> allocate_block();

    void free_blocks(const Vector<u32>& blocks);
    void free_block(u32 block);

    RefPtr<fs::Inode> create_inode(mode_t mode, dev_t dev, uid_t uid, gid_t gid);

private:
    FileSystem(Superblock* superblock, BlockDevice* drive);

    Superblock* m_superblock;
    BlockDevice* m_disk;

    Vector<BlockGroup*> m_block_groups;
};

}