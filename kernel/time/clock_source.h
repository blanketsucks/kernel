#pragma once

#include <kernel/common.h>

#include <std/string.h>

namespace kernel {

enum class ClockSourceType {
    HPET = 1,
    TSC  = 2
};

class ClockSource {
public:
    virtual ~ClockSource() = default;

    virtual StringView name() const = 0;
    virtual ClockSourceType type() const = 0;

    virtual u64 frequency() const = 0;

    virtual u64 read() const = 0;
    virtual u64 mask() const = 0;
};

}
