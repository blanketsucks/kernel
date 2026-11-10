#pragma once

#include <kernel/common.h>
#include <kernel/arch/tss.h>

#include <std/vector.h>
#include <std/optional.h>

namespace kernel {

class Process;
class Thread;

class Scheduler {
public:
    static void initialize();
    static void run();

    static Thread* create_idle_thread(int cpu);

    static pid_t generate_pid();

    static void yield();

    static void lock();
    static void unlock();

    static void add_process(Process*);

    static void queue(Thread*, Optional<Thread::State> with_state = {});
    static void dequeue(Thread*, Optional<Thread::State> with_state = {});

    static Process* get_process(pid_t id);

    static Thread* get_next_thread();
};

}
