#pragma once

#include <kernel/common.h>

namespace kernel {

class Blocker {
public:
    virtual ~Blocker() = default;

    virtual bool should_unblock() = 0;
};

class BooleanBlocker : public Blocker {
public:
    BooleanBlocker(bool value = false) : m_value(value) {}
    bool should_unblock() override { return m_value; }

    void set_value(bool value) { m_value = value; }

private:
    bool m_value;
};

class SleepBlocker : public Blocker {
public:
    SleepBlocker(i32 wake_time) : m_wake_time(wake_time) {}
    bool should_unblock() override;

private:
    i32 m_wake_time;
};

}