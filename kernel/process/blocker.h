#pragma once

#include <kernel/common.h>
#include <kernel/posix/sys/types.h>
#include <kernel/posix/time.h>

#include <std/time.h>

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
    SleepBlocker(Duration duration, clockid_t clock_id, bool is_absolute = false);

    bool should_unblock() override;

private:
    Duration m_deadline;
    clockid_t m_clock_id;  
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