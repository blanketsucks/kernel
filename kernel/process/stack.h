#pragma once

#include <kernel/common.h>

namespace kernel {

class Stack {
public:
    Stack() = default;
    Stack(void* stack, size_t size) : m_stack(reinterpret_cast<u32>(stack) + size), m_size(size) {}

    u32 value() const { return reinterpret_cast<u32>(m_stack); }
    u32 base() const { return reinterpret_cast<u32>(m_stack) + m_offset; }

    size_t size() const { return m_size; }
    size_t offset() const { return m_offset; }

    void push(u32 value) {
        m_stack -= sizeof(u32);
        m_offset += sizeof(u32);

        *reinterpret_cast<u32*>(m_stack) = value;
    }

private:
    u32 m_stack = 0;

    size_t m_size = 0;
    size_t m_offset = 0;
};

}