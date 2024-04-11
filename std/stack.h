#pragma once

#include <kernel/common.h>
#include <std/vector.h>

namespace std {

template<typename T>
class Stack {
public:
    using Iterator = typename Vector<T>::Iterator;
    using ConstIterator = typename Vector<T>::ConstIterator;

    Stack() = default;

    Stack(const Stack<T>& other) : buffer(other.buffer) {}

    void reserve(size_t capacity) { this->buffer.reserve(capacity); }
    void resize(size_t size) { this->buffer.resize(size); }

    void shrink_to_fit() { this->buffer.shrink_to_fit(); }

    void push(const T& value) { this->buffer.append(value); }

    T pop() {
        return this->buffer.take_last();
    }

    T& top() { return this->buffer.last(); }
    const T& top() const { return this->buffer.last(); }

    bool empty() const { return this->buffer.empty(); }
    size_t size() const { return this->buffer.size(); }

    void clear() { this->buffer.clear(); }

    Iterator begin() { return this->buffer.begin(); }
    Iterator end() { return this->buffer.end(); }

    ConstIterator begin() const { return this->buffer.begin(); }
    ConstIterator end() const { return this->buffer.end(); }

    Iterator find(const T& value) { return this->buffer.find(value); }
    ConstIterator find(const T& value) const { return this->buffer.find(value); }
    
    bool contains(const T& value) const { return this->buffer.contains(value); }

private:
    Vector<T> buffer;
};

}

using std::Stack;