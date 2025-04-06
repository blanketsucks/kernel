#pragma once

#include <kernel/devices/storage/device.h>

namespace kernel {

class StorageController {
public:
    virtual RefPtr<StorageDevice> device(size_t index) const = 0;
    virtual size_t devices() const = 0;
};

}