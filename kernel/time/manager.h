#pragma once

#include <kernel/common.h>
#include <kernel/posix/sys/types.h>

namespace kernel {

class TimeManager {
public:
    static void init();
    static TimeManager* instance();

    static time_t boot_time();

    static void tick();

private:
    TimeManager() = default;

    void initialize();

    void timer_tick();
};

}