#pragma once

#include <kernel/common.h>

#include <std/array.h>
#include <std/enums.h>
#include <kernel/posix/sys/types.h>

namespace kernel::ext2fs {

constexpr u16 MAGIC = 0xEF53;
constexpr ino_t ROOT_INODE = 2;
constexpr size_t MAX_NAME_SIZE = 255;

enum class FileSystemStatus : u16 {
    Clean,
    Error
};

enum class ErrorHandlingMethod : u16 {
    Ignore,
    RemountReadOnly,
    Panic
};

enum class OSID : u32 {
    Linux,
    GNUHurd,
    Masix,
    FreeBSD,
    Other
};

enum class OptionalFeature : u32 {
    Preallocate = 0x0001,               // Preallocate some number of blocks (Superblock.preallocated_directory_blocks) to a directory when creating a new one
    AFS = 0x0002,                       // AFS server inodes exist
    Journal = 0x0004,                   // File system has a journal (Ext3)
    InodeExtendedAttributes = 0x0008,   // Inodes have extended attributes
    Resize = 0x0010,                    // File system can resize itself for larger partitions
    DirectoryHashIndex = 0x0020         // Directories use hash index
};

enum class RequiredFeature : u32 {
    Compression   = 0x0001,             // Compression is used
    DirectoryType = 0x0002,             // Directory entries contain a type field
    ReplayJournal = 0x0004,             // File system needs to replay its journal
    JournalDevice = 0x0008,             // File system uses a journal device
};

enum class ReadOnlyFeature : u32 {
    SparseSuperblock = 0x0001,          // Sparse superblocks and group descriptor tables
    LargeFileSize = 0x0002,             // Large file size support (File system uses a 64-bit file size)
    BTreeDirectory = 0x0004             // Directory contents are stored in the form of a Binary Tree
};

MAKE_ENUM_BITWISE_OPS(OptionalFeature)
MAKE_ENUM_BITWISE_OPS(RequiredFeature)
MAKE_ENUM_BITWISE_OPS(ReadOnlyFeature)

enum class InodeType : u16 {
    FIFO = 0x1000,
    CharacterDevice = 0x2000,
    Directory = 0x4000,
    BlockDevice = 0x6000,
    RegularFile = 0x8000,
    SymbolicLink = 0xA000,
    UnixSocket = 0xC000
};

enum class InodePermission : u16 {
    OtherExecute = 0x0001,
    OtherWrite = 0x0002,
    OtherRead = 0x0004,
    GroupExecute = 0x0008,
    GroupWrite = 0x0010,
    GroupRead = 0x0020,
    UserExecute = 0x0040,
    UserWrite = 0x0080,
    UserRead = 0x0100,
    StickyBit = 0x0200,
    SetGroupID = 0x0400,
    SetUserID = 0x0800
};

enum class InodeFlags : u32 {
    SecureDelete = 0x00000001,
    KeepCopyOnDelete = 0x00000002,
    FileCompression = 0x00000004,
    SynchronousUpdates = 0x00000008,
    ImmutableFile = 0x00000010,
    AppendOnly = 0x00000020,
    NoDump = 0x00000040,
    LastAccessTime = 0x00000080,
    HashIndexDirectory = 0x00010000,
    AFSDirectory = 0x00020000,
    JournalFileData = 0x00040000
};

MAKE_ENUM_BITWISE_OPS(InodeType)
MAKE_ENUM_BITWISE_OPS(InodePermission)

struct Superblock {
    u32 total_inodes;
    u32 total_blocks;
    u32 superuser_blocks;
    u32 free_blocks;
    u32 free_inodes;
    u32 superblock_block;
    u32 block_size;            // log2(block size) - 10 (block size = 1024 << block_size)
    u32 fragment_size;         // log2(fragment size) - 10 (fragment size = 1024 << fragment_size)
    u32 blocks_per_group;
    u32 fragments_per_group;
    u32 inodes_per_group;
    u32 last_mount_time;       // In POSIX time
    u32 last_write_time;       // In POSIX time
    u16 times_mounted;         // Number of times the volume has been mounted since its last consistency check (fsck)
    u16 mounts_allowed;        // Number of mounts allowed before a consistency check (fsck) must be done
    u16 magic;
    FileSystemStatus state;
    ErrorHandlingMethod error_handling_method;
    u16 version_minor;
    u32 last_check;            // POSIX time of last consistency check (fsck)
    u32 forced_check_interval; // POSIX time interval between forced consistency checks (fsck)
    OSID os_id;
    u32 version_major;
    u16 reserved_user;
    u16 reserved_group;

    // Extended superblock fields
    u32 first_inode;
    u16 inode_size;
    u16 superblock_group;
    OptionalFeature optional_features;
    RequiredFeature required_features;
    ReadOnlyFeature ro_features;
    Array<u8, 16> uuid;
    Array<u8, 16> volume_name;
    Array<u8, 64> last_mount_path;
    u32 compression_algorithm;
    u8 preallocated_file_blocks; // Number of blocks to preallocate for files
    u8 preallocated_directory_blocks;
    u16 unused;
    Array<u8, 16> journal_id;
    u32 journal_inode;
    u32 journal_device;
    u32 orphan_inode_head;
    
    u8 padding[788];
} PACKED;

static_assert(sizeof(Superblock) == SECTOR_SIZE * 2);

struct BlockGroupDescriptor {
    u32 block_bitmap;
    u32 inode_bitmap;
    u32 inode_table;
    u16 free_blocks;
    u16 free_inodes;
    u16 dir_count;
    u8 unused[14];
} PACKED;

static_assert(sizeof(BlockGroupDescriptor) == 32);

struct Inode {
    u16 mode;
    u16 user_id;
    u32 size_lower;
    u32 last_access_time;
    u32 creation_time;
    u32 last_modification_time;
    u32 deletion_time;
    u16 group_id;
    u16 hard_link_count;
    u32 disk_sectors;
    u32 flags;
    u32 os_specific_value1;
    u32 block_pointers[12];
    u32 singly_indirect_block_pointer;
    u32 doubly_indirect_block_pointer;
    u32 triply_indirect_block_pointer;
    u32 generation_number;
    u32 extended_attribute_block;
    u32 size_upper;
    u32 fragment_block_address;
    u8 os_specific_value2[12];

    // Type occupies the upper 4 bits, permissions occupy the lower 12 bits
    InodeType type() const { return static_cast<InodeType>(mode & 0xF000); }
    InodePermission permissions() const { return static_cast<InodePermission>(mode & 0x0FFF); }
} PACKED;

static_assert(sizeof(Inode) == 128);

// This doesn't incldue the name because we don't know the length of it until we actually read the entry.
struct DirEntry {
    u32 inode;
    u16 size;
    u8 name_length;
    u8 type_indicator;
    char name[];
} PACKED;

}