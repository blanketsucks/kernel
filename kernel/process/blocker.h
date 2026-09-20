#pragma once

#include <kernel/common.h>
#include <kernel/posix/sys/types.h>
#include <kernel/posix/time.h>
#include <kernel/fs/fd.h>

#include <std/time.h>

namespace kernel {

class Thread;
class Process;

class Blocker : public std::RefCounted {
public:
    virtual ~Blocker() = default;

    virtual bool should_unblock() = 0;

    void wait();
};

class BooleanBlocker : public Blocker {
public:
    static RefPtr<BooleanBlocker> create(bool initial_value = false) {
        return RefPtr(new BooleanBlocker(initial_value));
    }

    bool should_unblock() override { return m_value; }
    void set_value(bool value) { m_value = value; }
    
private:
    BooleanBlocker(bool value = false) : m_value(value) {}

    bool m_value;
};

class SleepBlocker : public Blocker {
public:
    static RefPtr<SleepBlocker> create(Duration duration, clockid_t clock_id, bool is_absolute = false) {
        return RefPtr(new SleepBlocker(duration, clock_id, is_absolute));
    }

    bool should_unblock() override;

private:
    SleepBlocker(Duration duration, clockid_t clock_id, bool is_absolute = false);

    Duration m_deadline;
    clockid_t m_clock_id;
};

class WaitBlocker : public Blocker {
public:
    static RefPtr<WaitBlocker> create(Thread* thread, pid_t pid);
    
    static void try_wake_all(Process*, int status);
    void try_wake(Process*, int status);

    bool should_unblock() override { return m_ready; } 
    int status() const { return m_status; }

private:
    WaitBlocker(Thread* thread, pid_t pid) : m_thread(thread), m_pid(pid) {}

    Thread* m_thread;
    pid_t m_pid;

    int m_status;
    bool m_ready = false;
};

class FileBlocker : public Blocker {
public:
    static RefPtr<FileBlocker> create(RefPtr<fs::FileDescriptor> fd, int options) {
        return RefPtr(new FileBlocker(move(fd), options));
    }

    int options() const { return m_options; }
    bool should_unblock() override;

private:
    FileBlocker(RefPtr<fs::FileDescriptor> fd, int options) : m_fd(move(fd)), m_options(options) {}

    RefPtr<fs::FileDescriptor> m_fd;
    int m_options;
};

}