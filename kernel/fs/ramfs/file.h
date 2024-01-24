#if 0

#include <kernel/common.h>
#include <kernel/fs/file.h>

#include <std/string.h>

namespace kernel::ramfs {

struct Node;
class FileSystem;

class File : public fs::File {
public:
    File(FileSystem* fs, Node* node);

    size_t read(void* buffer, size_t size, size_t offset) override;
    size_t write(const void* buffer, size_t size, size_t offset) override;

private:
    Node* m_node;
    FileSystem* m_fs;
};

}

#endif