#pragma once

#include <kernel/common.h>

#include <kernel/fs/ext2fs/ext2.h>
#include <kernel/fs/ext2fs/inode.h>
#include <kernel/fs/ext2fs/block_group.h>
#include <kernel/fs/filesystem.h>

#include <kernel/devices/block.h>

#include <std/function.h>
#include <std/memory.h>
#include <std/hash_map.h>

namespace kernel::ext2fs {

class FileSystem : public fs::FileSystem {
public:
    static FileSystem* create(devices::BlockDevice* disk);
    static FileSystem* instance();

    RefPtr<fs::Inode> inode(ino_t inode) override;
    ino_t root() const override;

    u8 id() const override { return 0x83; }
    StringView name() const override { return "ext2"; }

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

    RefPtr<InodeEntry> create_inode(mode_t mode, uid_t uid, gid_t gid);

private:
    FileSystem(Superblock* superblock, devices::BlockDevice* drive);

    Superblock* m_superblock;
    devices::BlockDevice* m_disk;

    Vector<BlockGroup*> m_block_groups;
    static FileSystem* s_instance;
};

}