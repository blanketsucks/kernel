#pragma once

#include <kernel/common.h>

namespace std {

constexpr struct nullopt_t {
    constexpr nullopt_t() {}
} None;

template<typename T>
class Optional {
public:
    Optional() = default;
    Optional(nullopt_t) : m_has_value(false) {}
    Optional(const T& value) : m_value(value), m_has_value(true) {}
    Optional(T&& value) : m_value(move(value)), m_has_value(true) {}

    Optional(const Optional& other) : m_value(other.m_value), m_has_value(other.m_has_value) {}
    Optional(Optional&& other) : m_value(move(other.m_value)), m_has_value(other.m_has_value) {}

    Optional& operator=(const Optional& other) {
        m_value = other.m_value;
        m_has_value = other.m_has_value;
        return *this;
    }

    Optional& operator=(Optional&& other) {
        m_value = move(other.m_value);
        m_has_value = other.m_has_value;
        return *this;
    }

    bool has_value() const { return m_has_value; }

    T& value() { return m_value; }
    const T& value() const { return m_value; }

    T& value_or(T& default_value) { return m_has_value ? m_value : default_value; }
    const T& value_or(const T& default_value) const { return m_has_value ? m_value : default_value; }

    void clear() { m_has_value = false; }

private:
    T m_value;
    bool m_has_value = false;
};

}

using std::Optional;