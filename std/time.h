#pragma once

#include <std/types.h>

#ifdef __KERNEL__
    #include <kernel/posix/time.h>
#else
    #include <time.h>
#endif

namespace std {

class Duration {
public:
    constexpr Duration() = default;
    constexpr explicit Duration(i64 seconds, u32 nanoseconds) : m_seconds(seconds), m_nanoseconds(nanoseconds) {}

    static constexpr Duration zero() {
        return Duration(0, 0);
    }

    static constexpr Duration from_seconds(u64 seconds) {
        return Duration(seconds, 0);
    }

    static constexpr Duration from_milliseconds(u64 milliseconds) {
        i64 seconds = milliseconds / 1'000;
        milliseconds %= 1'000;

        return Duration(seconds, milliseconds * 1'000'000);
    }

    static constexpr Duration from_microseconds(u64 microseconds) {
        i64 seconds = microseconds / 1'000'000;
        microseconds %= 1'000'000;

        return Duration(seconds, microseconds * 1'000);
    }

    static constexpr Duration from_nanoseconds(u64 nanoseconds) {
        i64 seconds = nanoseconds / 1'000'000'000;
        nanoseconds %= 1'000'000'000;

        return Duration(seconds, static_cast<u32>(nanoseconds));
    }

    static constexpr Duration from_timespec(const timespec& ts) {
        return Duration(ts.tv_sec, static_cast<u32>(ts.tv_nsec));
    }

    constexpr Duration operator+(const Duration& other) const {
        u32 nsecs = m_nanoseconds + other.m_nanoseconds;
        i64 secs = m_seconds + other.m_seconds + (nsecs / 1'000'000'000);

        nsecs %= 1'000'000'000;
        return Duration(secs, nsecs);
    }

    constexpr Duration operator+=(const Duration& other) {
        *this = *this + other;
        return *this;
    }

    constexpr Duration operator-(const Duration& other) const {
        i64 secs = m_seconds - other.m_seconds;
        if (secs < 0) {
            return zero();
        }

        u32 nsecs = 0;
        if (other.m_nanoseconds > m_nanoseconds) {
            if (secs == 0) {
                return zero();
            }

            secs--;
            nsecs = 1'000'000'000 + m_nanoseconds - other.m_nanoseconds;
        } else {
            nsecs = m_nanoseconds - other.m_nanoseconds;
        }

        return Duration(secs, nsecs);
    }

    constexpr bool operator==(const Duration& other) const = default;
    constexpr bool operator!=(const Duration& other) const = default;

    constexpr bool operator<(const Duration& other) const {
        return m_seconds < other.m_seconds || (m_seconds == other.m_seconds && m_nanoseconds < other.m_nanoseconds);
    }

    constexpr bool operator<=(const Duration& other) const {
        return m_seconds < other.m_seconds || (m_seconds == other.m_seconds && m_nanoseconds <= other.m_nanoseconds);
    }

    constexpr bool operator>(const Duration& other) const {
        return m_seconds > other.m_seconds || (m_seconds == other.m_seconds && m_nanoseconds > other.m_nanoseconds);
    }

    constexpr bool operator>=(const Duration& other) const {
        return m_seconds > other.m_seconds || (m_seconds == other.m_seconds && m_nanoseconds >= other.m_nanoseconds);
    }

    constexpr i64 seconds() const {
        return m_seconds;
    }

    constexpr u32 nanoseconds() const {
        return m_nanoseconds;
    }

private:
    i64 m_seconds = 0;
    u32 m_nanoseconds = 0;
};

namespace time_literals {

constexpr Duration operator"" _s(unsigned long long seconds) {
    return Duration::from_seconds(seconds);
}

constexpr Duration operator"" _ms(unsigned long long milliseconds) {
    return Duration::from_milliseconds(milliseconds);
}

constexpr Duration operator"" _us(unsigned long long microseconds) {
    return Duration::from_microseconds(microseconds);
}

constexpr Duration operator"" _ns(unsigned long long nanoseconds) {
    return Duration::from_nanoseconds(nanoseconds);
}

}

}

using std::Duration;