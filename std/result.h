#pragma once

#include <kernel/common.h>

#include <kernel/posix/errno.h>
#include <std/utility.h>

#define TRY(expr)                       \
    ({                                  \
        auto result = (expr);           \
        if (result.is_err()) {          \
            return result.error();      \
        }                               \
        result.value();                 \
    })

namespace std {

template<typename T, typename E>
class Result {
public:
    Result(const T& value) : m_value(value), m_error(), has_value(true) {}
    Result(T&& value) : m_value(move(value)), m_error(), has_value(true) {}

    Result(const E& error) : m_value(), m_error(error), has_value(false) {}

    bool is_ok() const { return has_value; }
    bool is_err() const { return !is_ok(); }

    const T& value() const { return m_value; }
    T& value() { return m_value; }

    const E& error() const { return m_error; }
    E& error() { return m_error; }

private:
    T m_value;
    E m_error;

    bool has_value;
};

template<typename E>
class Result<void, E> {
public:
    Result() : m_error(), has_err(false) {}
    Result(const E& error) : m_error(error), has_err(true) {}

    bool is_err() const { return has_err; }
    bool is_ok() const { return !is_err(); }

    void value() const { return; } // Added here to make it possible to use the TRY() macro

    const E& error() const { return m_error; }
    E& error() { return m_error; }
private:
    E m_error;
    bool has_err;
};

class Error {
public:
    Error() : m_msg("Unknown error"), m_errno(0) {}
    Error(int errno) : m_msg(nullptr), m_errno(errno) {}
    Error(const char* message) : m_msg(message) {}

    const char* message() const { return m_msg; }
    int errno() const { return m_errno; }

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