#pragma once

#include <kernel/common.h>
#include <kernel/panic.h>
#include <kernel/posix/errno.h>

#include <std/utility.h>
#include <std/kmalloc.h>

#define TRY(expr)                       \
    ({                                  \
        auto result = (expr);           \
        if (result.is_err()) {          \
            return result.error();      \
        }                               \
        result.release_value();         \
    })

namespace std {

template<typename T, typename E>
class Result {
public:
    Result(const T& value) : m_has_value(true) {
        new (&m_value_storage) T(value);
    }

    Result(T&& value) : m_has_value(true) {
        new (&m_value_storage) T(move(value));
    }

    Result(const E& error) : m_has_value(false) {
        new (&m_error_storage) E(error);
    }

    Result(E&& error) : m_has_value(false) {
        new (&m_error_storage) E(move(error));
    }

    Result(const Result& other) : m_has_value(other.m_has_value) {
        if (m_has_value) {
            new (&m_value_storage) T(other.m_value_storage);
        } else {
            new (&m_error_storage) E(other.m_error_storage);
        }
    }

    Result(Result&& other) : m_has_value(other.m_has_value) {
        if (m_has_value) {
            new (&m_value_storage) T(move(other.m_value_storage));
        } else {
            new (&m_error_storage) E(move(other.m_error_storage));
        }

        other.m_has_value = false;
    }

    ~Result() {
        this->reset();
    }

    Result& operator=(const Result& other) {
        if (this == &other) {
            return *this;
        }

        this->reset();
        m_has_value = other.m_has_value;
        if (m_has_value) {
            new (&m_value_storage) T(other.m_value_storage);
        } else {
            new (&m_error_storage) E(other.m_error_storage);
        }

        return *this;
    }

    Result& operator=(Result&& other) {
        if (this == &other) {
            return *this;
        }

        this->reset();
        m_has_value = other.m_has_value;
        if (m_has_value) {
            new (&m_value_storage) T(move(other.m_value_storage));
        } else {
            new (&m_error_storage) E(move(other.m_error_storage));
        }

        other.m_has_value = false;
        return *this;
    }

    bool is_ok() const { return m_has_value; }
    bool is_err() const { return !is_ok(); }

    const T& value() const { return m_value_storage; }
    T& value() { return m_value_storage; }

    const E& error() const { return m_error_storage; }
    E& error() { return m_error_storage; }

    T release_value() {
        // TODO: Handle the case where this is an error
        T value = move(m_value_storage);
        m_has_value = false;

        m_value_storage.~T();
        return value;
    }

    E release_error() {
        // TODO: Handle the case where this is a value
        E error = move(m_error_storage);
        m_has_value = true;

        m_error_storage.~E();
        return error;
    }

    T& unwrap() {
        if (this->is_err()) {
            kernel::panic("Result::unwrap() called on an error");
        }

        return m_value_storage;
    }

    void reset() {
        if (m_has_value) {
            m_value_storage.~T();
        } else {
            m_error_storage.~E();
        }

        m_has_value = false;
    }

private:
    union {
        T m_value_storage;
        E m_error_storage;
    };

    bool m_has_value;
};

template<typename E>
class Result<void, E> {
public:
    Result() : m_error(), has_err(false) {}
    Result(const E& error) : m_error(error), has_err(true) {}

    bool is_err() const { return has_err; }
    bool is_ok() const { return !is_err(); }

    void value() const { return; }
    void release_value() { return; }

    const E& error() const { return m_error; }
    E& error() { return m_error; }
    
private:
    E m_error;
    bool has_err;
};

class Error {
public:
    Error() : m_msg("Unknown error"), m_errno(0) {}
    
    Error(int err) : m_msg(nullptr), m_errno(err) {}
    Error(const char* message) : m_msg(message) {}

    const char* message() const { return m_msg; }
    int code() const { return m_errno; }

private:
    const char* m_msg;
    int m_errno;
};

template<typename T>
using ErrorOr = Result<T, Error>;

}

using std::Result;
using std::Error;
using std::ErrorOr;