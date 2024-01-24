#pragma once

#include <kernel/common.h>

#include <kernel/fs/ext2fs/ext2.h>
#include <kernel/fs/ext2fs/inode.h>
#include <kernel/fs/filesystem.h>

#include <kernel/devices/pata.h>

#include <std/memory.h>
#include <std/hash_map.h>

namespace kernel::ext2fs {

class FileSystem : public fs::FileSystem {
public:
    static FileSystem* create(devices::PATADevice* disk);
    static FileSystem* instance();

    RefPtr<fs::Inode> inode(ino_t inode) override;
    ino_t root() const override;

    u8 id() const override { return 0x83; }
    StringView name() const override { return "ext2"; }

    Superblock const* superblock() const { return m_superblock; }

    u32 block_size() const { return 1024 << m_superblock->block_size; }
    u32 block_group_count() const { return (m_superblock->total_blocks + m_superblock->blocks_per_group - 1) / m_superblock->blocks_per_group; }

    void flush_superblock() const;

    BlockGroupDescriptor* get_block_group(u32 index);

    void read_block_group(u32 index, BlockGroupDescriptor* group) const;
    void write_block_group(u32 index, const BlockGroupDescriptor* group) const;

    void read_block(u32 block, u8* buffer) const;
    void read_blocks(u32 block, u32 count, u8* buffer) const;

    void write_block(u32 block, const u8* buffer) const;
    void write_blocks(u32 block, u32 count, const u8* buffer) const;

    RefPtr<InodeEntry> create_inode(mode_t mode, uid_t uid, gid_t gid);

private:
    FileSystem(Superblock* superblock, devices::PATADevice* drive);

    Superblock* m_superblock;
    devices::PATADevice* m_disk;

    Vector<BlockGroupDescriptor*> m_block_group_descriptors;
    static FileSystem* s_instance;
};

}