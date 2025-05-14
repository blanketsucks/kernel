#pragma once

#include <kernel/common.h>
#include <kernel/posix/sys/types.h>

namespace kernel::rtc {

void init();
time_t boot_time();

time_t now();

}