#pragma once

#include <kernel/common.h>

#include <std/memory.h>

namespace kernel {

class StorageDevice;

class StorageController {
public:
    virtual RefPtr<StorageDevice> device(size_t index) const = 0;
    virtual size_t devices() const = 0;
};

}