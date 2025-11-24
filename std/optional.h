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

    Optional(const T& value) : m_has_value(true) {
        new (&m_storage) T(value);
    }

    Optional(T&& value) : m_has_value(true) {
        new (&m_storage) T(move(value));
    }

    Optional(const Optional& other) : m_has_value(other.m_has_value) {
        if (m_has_value) {
            new (&m_storage) T(other.value());
        }
    }

    Optional(Optional&& other) : m_has_value(other.m_has_value) {
        if (m_has_value) {
            new (&m_storage) T(move(other.value()));
            other.reset();
        }
    }

    Optional& operator=(const Optional& other) {
        if (this == &other) {
            return *this;
        }

        reset();
        m_has_value = other.m_has_value;
        if (m_has_value) {
            new (&m_storage) T(other.value());
        }

        return *this;
    }

    Optional& operator=(Optional&& other) {
        if (this == &other) {
            return *this;
        }

        reset();
        m_has_value = other.m_has_value;
        if (m_has_value) {
            new (&m_storage) T(move(other.value()));
            other.reset();
        }
        
        return *this;
    }

    T* operator->() { return reinterpret_cast<T*>(&m_storage); }
    const T* operator->() const { return reinterpret_cast<const T*>(&m_storage); }

    bool has_value() const { return m_has_value; }
    explicit operator bool() const { return m_has_value; }

    T& value() { return *reinterpret_cast<T*>(&m_storage); }
    const T& value() const { return *reinterpret_cast<const T*>(&m_storage); }

    T& value_or(T& default_value) { return m_has_value ? value() : default_value; }
    const T& value_or(const T& default_value) const { return m_has_value ? value() : default_value; }

    T release_value() {
        T released_value = move(value());
        reset();

        return released_value;
    }

    void reset() {
        if (m_has_value) {
            value().~T();
            m_has_value = false;
        }
    }

private:
    alignas(T) unsigned char m_storage[sizeof(T)];
    bool m_has_value = false;
};

}

using std::None;
using std::Optional;