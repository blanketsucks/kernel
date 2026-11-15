#include <kernel/process/wait_queue.h>
#include <kernel/process/scheduler.h>
#include <kernel/time/manager.h>
#include <kernel/process/threads.h>
#include <std/asserts.h>

namespace kernel {

Deadline Deadline::after(Duration time, clockid_t clock_id) {
    if (time == Duration::max()) {
        return never();
    }

    return { time + TimeManager::query_time(clock_id), clock_id, Kind::At };
}

bool Deadline::is_expired() const {
    return TimeManager::query_time(m_clock_id) >= m_at;
}

WaitEntry::~WaitEntry() {
    if (m_queue) {
        m_queue->cancel(*this);
    }
}

void WaitEntry::wake_up() {
    m_woken = true;
    m_thread->wake_up();
}

void WaitEntry::timeout() {
    m_timed_out = true;
    m_thread->wake_up();
}

void WaitQueue::prepare(WaitEntry& entry) {
    ASSERT(entry.m_queue == nullptr);

    entry.m_queue  = this;
    entry.m_thread = Thread::current();

    SpinLockGuard guard(m_lock);
    m_list.append(entry);
}

void WaitQueue::cancel(WaitEntry& entry) {
    ASSERT(entry.m_queue == this);

    SpinLockGuard guard(m_lock);

    m_list.remove(entry);
    entry.m_queue = nullptr;
}

WaitResult WaitQueue::wait(WaitEntry& entry, Deadline deadline) {
    ASSERT(entry.m_queue == this);

    if (deadline.is_now()) {
        SpinLockGuard guard(m_lock);
        bool woken = entry.m_woken;

        return woken ? WaitResult::Ok : WaitResult::Timeout;
    } else if (!deadline.is_infinite()) {
        entry.m_deadline = deadline;
        TimeManager::add_waiter(entry);
    }

    InterruptState state = m_lock.lock();
    while (!entry.is_done()) {
        entry.m_thread->set_state(Thread::Blocked);
        m_lock.unlock(InterruptState::Disabled);

        Scheduler::yield();
        m_lock.lock();
    }

    WaitResult result = entry.m_woken ? WaitResult::Ok : WaitResult::Timeout;
    m_lock.unlock(state);

    if (!deadline.is_infinite()) {
        TimeManager::cancel_waiter(entry);
    }

    return result;
}

void WaitQueue::wake_one() {
    SpinLockGuard guard(m_lock);

    auto& entry = m_list.first();
    entry.wake_up();

    m_list.remove(entry);
}

void WaitQueue::wake_all() {
    SpinLockGuard guard(m_lock);
    for (auto& entry : m_list) {
        entry.wake_up();
        m_list.remove(entry);
    }
}

void WaitQueue::on_timeout(WaitEntry& entry) {
    SpinLockGuard guard(m_lock);

    entry.timeout();
    m_list.remove(entry);
}

}
