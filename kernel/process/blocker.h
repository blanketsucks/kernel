#pragma once

#include <kernel/common.h>
#include <kernel/posix/sys/types.h>

namespace kernel {

class Thread;
class Process;

class Blocker {
public:
    virtual ~Blocker() = default;

    virtual bool should_unblock() = 0;

    void wait();
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

class WaitBlocker : public Blocker {
public:
    WaitBlocker(Thread* thread, pid_t pid) : m_thread(thread), m_pid(pid) {}

    static WaitBlocker* create(Thread* thread, pid_t pid);
    
    static void try_wake_all(Process*, int status);
    void try_wake(Process*, int status);

    bool should_unblock() override {
        return m_ready;
    }

    int status() const {
        return m_status;
    }

private:
    Thread* m_thread;
    pid_t m_pid;

    int m_status;
    bool m_ready = false;
};

}