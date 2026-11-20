#pragma once

#include <std/string_view.h>
#include <std/function.h>

namespace kernel {

enum class EventTimerType : u8 {
    PIT   = 0,
    HPET  = 1,
    LAPIC = 2
};

class EventTimer {
public:
    virtual ~EventTimer() = default;

    virtual StringView name() const = 0;
    virtual EventTimerType type() const = 0;

    virtual u64 frequency() const = 0;

    virtual bool supports_oneshot() const = 0;
    virtual bool supports_periodic() const = 0;

    virtual void start_periodic(u64 hz) = 0;
    virtual void start_oneshot(u64 ticks) = 0;

    virtual void stop() = 0;

    void set_callback(Function<void()> cb) { m_callback = move(cb); }

protected:
    void dispatch() {
        if (m_callback) {
            m_callback();
        }
    }

private:
    Function<void()> m_callback = nullptr;
};

}
