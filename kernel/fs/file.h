#pragma once

#include <kernel/common.h>
#include <kernel/posix/sys/stat.h>

namespace kernel::fs {

class File {
public:
    virtual ~File() = default;

    virtual size_t read(void* buffer, size_t size, size_t offset) = 0;
    virtual size_t write(const void* buffer, size_t size, size_t offset) = 0;

};

}