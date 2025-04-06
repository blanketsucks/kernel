#pragma once

#include <kernel/common.h>

namespace kernel {

class PCSpeaker {
public:
    static void beep(u32 frequency);
    static void stop();
};

}