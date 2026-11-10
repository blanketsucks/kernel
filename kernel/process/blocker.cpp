#include <kernel/process/blocker.h>
#include <kernel/process/scheduler.h>
#include <kernel/process/threads.h>
#include <kernel/process/process.h>
#include <kernel/posix/sys/wait.h>
#include <kernel/time/manager.h>

namespace kernel {

static SpinLockResource<Vector<WaitBlocker*>> s_wait_blockers;

void Blocker::wait() {
    auto* thread = Thread::current();
    thread->block(RefPtr<Blocker>(this));
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

    return remaining <= Duration::zero();
}

RefPtr<WaitBlocker> WaitBlocker::create(Thread* thread, pid_t pid) {
    auto blocker = RefPtr(new WaitBlocker(thread, pid));
    s_wait_blockers.with([&](Vector<WaitBlocker*> blockers) { blockers.append(blocker.ptr()); });

    return blocker;
}

void WaitBlocker::try_wake_all(Process* process, int status) {
    s_wait_blockers.with([&](Vector<WaitBlocker*> blockers) {
        Vector<WaitBlocker*> to_remove;
        for (auto& blocker : blockers) {
            if (!blocker) {
                continue;
            }

            if (blocker->try_wake(process, status)) {
                to_remove.append(blocker);
            }
        }

        for (auto& blocker : to_remove) {
            blockers.remove(blocker);
        }
    });
}

bool WaitBlocker::try_wake(Process* process, int status) {
    if (m_pid != process->id()) {
        return false;
    }

    m_status = __WIFEXITED | status;
    m_ready = true;

    return true;
}

bool FileBlocker::should_unblock() {
    if (m_options & O_RDONLY) {
        return m_fd->can_read();
    } else if (m_options & O_WRONLY) {
        return m_fd->can_write();
    }

    return false;
}

}
