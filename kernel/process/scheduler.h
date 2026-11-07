#pragma once

#include <kernel/common.h>
#include <kernel/arch/tss.h>

#include <std/vector.h>

namespace kernel {

class Process;
class Thread;

class Scheduler {
public:
    static void initialize();
    static void run();

    static Thread* create_idle_thread(int cpu);

    static pid_t generate_pid();

    static void invoke_async();
    static bool is_invoked_async();

    static void lock();
    static void unlock();
    static bool is_locked();

    static void yield();

    static void add_process(Process*);

    static void queue(Thread*);
    static void dequeue(Thread*);

    static Process* current_process();
    static Thread* current_thread();

    static Process* get_process(pid_t id);

    static Thread* get_next_thread();
};

}
