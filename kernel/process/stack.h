#pragma once

#include <kernel/common.h>

#include <std/cstring.h>
#include <std/string.h>

namespace kernel {

class Stack {
public:
    Stack() = default;
    Stack(
        void* stack, size_t size
    ) : m_stack(reinterpret_cast<u32>(stack) + size), m_top(m_stack), m_size(size) {}

    u32 value() const { return reinterpret_cast<u32>(m_stack); }
    u32 top() const { return m_top; }

    size_t size() const { return m_size; }
    size_t offset() const { return m_offset; }

    template<typename T>
    void push(T value) {
        m_stack -= sizeof(T);
        m_offset += sizeof(T);

        *reinterpret_cast<T*>(m_stack) = value;
    }

private:
    u32 m_stack = 0;
    u32 m_top;

    size_t m_size = 0;
    size_t m_offset = 0;
};

template<> inline void Stack::push(const char* value) {
    size_t length = std::strlen(value) + 1;

    m_stack -= length;
    m_offset += length;

    memcpy(reinterpret_cast<void*>(m_stack), value, length);
}

template<> inline void Stack::push(String value) {
    m_stack -= value.size();
    m_offset += value.size();

    memcpy(reinterpret_cast<void*>(m_stack), value.data(), value.size());

}


}