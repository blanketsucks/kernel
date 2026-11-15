#pragma once

#include <kernel/sync/spinlock.h>

#include <std/time.h>
#include <std/intrusive_list.h>

namespace kernel {

class Thread;
class WaitQueue;

// TODO: Move this to somewhere else
class Deadline {
public:
    Deadline() = default;

    enum class Kind {
        Infinite,
        Now,
        At
    };

    DEFAULT_COPY(Deadline);
    DEFAULT_MOVE(Deadline);

    static Deadline never() { return { {}, 0, Kind::Infinite }; }
    static Deadline now() { return { {}, 0, Kind::Now }; }

    static Deadline at(Duration instant, clockid_t clock_id) { return { instant, clock_id, Kind::At }; }
    static Deadline after(Duration time, clockid_t clock_id = CLOCK_MONOTONIC);

    Duration at() const { return m_at; }
    clockid_t clock_id() const { return m_clock_id; }

    bool is_infinite() const { return m_kind == Kind::Infinite; }
    bool is_now() const { return m_kind == Kind::Now; }

    bool is_zero() const { return m_at == Duration::zero(); }

    bool is_expired() const;

private:
    Deadline(Duration at, clockid_t clock_id, Kind kind) : m_at(at), m_clock_id(clock_id), m_kind(kind) {}

    Duration m_at;
    clockid_t m_clock_id;

    Kind m_kind;
};

enum class WaitResult {
    Ok,
    Timeout
};

class WaitEntry {
public:
    NO_COPY(WaitEntry);
    NO_MOVE(WaitEntry);

    WaitEntry() = default;
    ~WaitEntry();

private:
    friend class WaitQueue;
    friend class TimeManager;

    void wake_up();
    void timeout();

    bool is_done() const { return m_woken || m_timed_out; }

    WaitQueue* m_queue = nullptr;
    Thread* m_thread   = nullptr;

    bool m_woken     = false;
    bool m_timed_out = false;

    Deadline m_deadline = Deadline::never();

    std::ListNode m_queue_node;
    std::ListNode m_timer_node;
};

class WaitQueue {
public:
    void prepare(WaitEntry&);
    void cancel(WaitEntry&);

    WaitResult wait(WaitEntry&, Deadline deadline = Deadline::never());
    WaitResult wait(WaitEntry& entry, Duration timeout) {
        return this->wait(entry, Deadline::after(timeout));
    }

    template<typename F>
    WaitResult prepare_and_wait(F&& begin, Deadline deadline = Deadline::never()) {
        WaitEntry entry;
        this->prepare(entry);

        begin();
        return this->wait(entry, deadline);
    }

    template<typename F>
    WaitResult wait_until(F&& ready, Deadline deadline = Deadline::never()) {
        for (;;) {
            WaitEntry entry;
            this->prepare(entry);

            if (ready()) {
                this->cancel(entry);
                return WaitResult::Ok;
            }

            if (this->wait(entry, deadline) == WaitResult::Timeout) {
                return WaitResult::Timeout;
            }
        }
    }

    void wake_one();
    void wake_all();

private:
    friend class TimeManager;

    using EntryList = std::IntrusiveList<&WaitEntry::m_queue_node>;

    void on_timeout(WaitEntry&);

    SpinLock m_lock;
    EntryList m_list;
};

}
