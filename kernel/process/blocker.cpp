#include <kernel/process/blocker.h>
#include <kernel/process/scheduler.h>
#include <kernel/process/threads.h>
#include <kernel/process/process.h>
#include <kernel/posix/sys/wait.h>
#include <kernel/time/manager.h>

namespace kernel {

static Vector<WaitBlocker*> s_wait_blockers;

void Blocker::wait() {
    auto* thread = Thread::current();
    thread->block(this);
}

SleepBlocker::SleepBlocker(Duration duration, clockid_t clock_id, bool is_absolute) : m_clock_id(clock_id) {
    if (is_absolute) {
        m_deadline = duration;
    } else {
        m_deadline = TimeManager::query_time(clock_id) + duration;
    }
}

bool SleepBlocker::should_unblock() {
    Duration current = TimeManager::query_time(m_clock_id);
    Duration remaining = m_deadline - current;

    if (remaining <= Duration::zero()) {
        return true;
    }

    return false;
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