#pragma once

#include <kernel/common.h>
#include <kernel/arch/tss.h>

#include <std/vector.h>

namespace kernel {

class Process;
class Thread;

u32 generate_id();

class Scheduler {
public:
    static void init();

    static void invoke_async();
    static bool is_invoked_async();

    static void lock();
    static void unlock();
    static bool is_locked();

    static void yield(bool if_idle = false);

    static void add_process(Process*);
    static void queue(Thread*);

    static Process* current_process();
    static Thread* current_thread();

    static Process* get_process(pid_t id);

    static void set_current_thread(Thread*);

    static Thread* next_thread();
    static Thread* set_next_thread(Thread*);

    static Thread* get_next_thread();
};

class ScopedSchedulerLock {
public:
    ScopedSchedulerLock() { Scheduler::lock(); }
    ~ScopedSchedulerLock() { Scheduler::unlock(); }
};

}