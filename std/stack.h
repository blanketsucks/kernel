#pragma once

#include <kernel/common.h>
#include <std/linked_list.h>

namespace std {

template<typename T>
class Stack {
public:
    using Iterator = typename SinglyLinkedList<T>::Iterator;
    using ConstIterator = typename SinglyLinkedList<T>::ConstIterator;

    Stack() = default;

    Stack(const Stack<T>& other) : m_list(other.m_list) {}

    void push(const T& value) { m_list.prepend(value); }
    T pop() { return m_list.take_first(); }

    T& top() { return m_list.first(); }
    const T& top() const { return m_list.first(); }

    bool empty() const { return m_list.empty(); }
    size_t size() const { return m_list.size(); }

    void clear() { m_list.clear(); }

    Iterator begin() { return m_list.begin(); }
    Iterator end() { return m_list.end(); }

    ConstIterator begin() const { return m_list.begin(); }
    ConstIterator end() const { return m_list.end(); }

    Iterator find(const T& value) { return m_list.find(value); }
    ConstIterator find(const T& value) const { return m_list.find(value); }
    
    bool contains(const T& value) const { return m_list.contains(value); }

private:
    SinglyLinkedList<T> m_list;
};

}

using std::Stack;