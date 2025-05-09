#pragma once

#include <kernel/pci/device.h>
#include <std/function.h>

namespace kernel::pci {

using EnumerationCallback = Function<void(Device)>;

void enumerate(EnumerationCallback&& callback);

}