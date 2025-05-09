#include <kernel/process/blocker.h>
#include <kernel/process/scheduler.h>
#include <kernel/process/threads.h>
#include <kernel/process/process.h>
#include <kernel/posix/sys/wait.h>
#include <kernel/time/rtc.h>

namespace kernel {

static Vector<WaitBlocker*> s_wait_blockers;

void Blocker::wait() {
    auto* thread = Scheduler::current_thread();
    thread->block(this);
}

bool SleepBlocker::should_unblock() {
    return m_wake_time <= rtc::now();
}

WaitBlocker* WaitBlocker::create(Thread* thread, pid_t pid) {
    auto* blocker = new WaitBlocker(thread, pid);
    s_wait_blockers.append(blocker);

    return blocker;
}

void WaitBlocker::try_wake_all(Process* process, int status) {
    for (auto& blocker : s_wait_blockers) {
        if (blocker) {
            blocker->try_wake(process, status);
        }
    }
}

void WaitBlocker::try_wake(Process* process, int status) {
    if (m_pid != process->id()) {
        return;
    }

    m_status = __WIFEXITED | status;
    m_ready = true;
}

}