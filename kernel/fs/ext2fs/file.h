#pragma once

#include <kernel/common.h>

#include <kernel/fs/file.h>

namespace kernel::ext2fs {

class InodeEntry;
class FileSystem;

class File : public fs::File {
public:
    File(FileSystem* fs, InodeEntry* inode);

    size_t read(void* buffer, size_t size, size_t offset) override;
    size_t write(const void* buffer, size_t size, size_t offset) override;

private:
    InodeEntry* m_inode;
    FileSystem* m_fs;
};

}