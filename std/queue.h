#pragma once

#include <std/linked_list.h>

namespace std {

template<typename T>
class Queue {
public:
    using Iterator = typename DoublyLinkedList<T>::Iterator;
    using ConstIterator = typename DoublyLinkedList<T>::ConstIterator;

    Queue() = default;
    Queue(const Queue<T>& other) : m_list(other.m_list) {}

    void enqueue(const T& value) { m_list.append(value); }
    T dequeue() { return m_list.take_first(); }

    T& front() { return m_list.first(); }
    const T& front() const { return m_list.first(); }

    T& back() { return m_list.last(); }
    const T& back() const { return m_list.last(); }

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

    void remove(const T& value) { m_list.remove(value); }
    void remove(Iterator iterator) { m_list.remove(iterator); }
    void remove(ConstIterator iterator) { m_list.remove(iterator); }

private:
    DoublyLinkedList<T> m_list;
};

}

using std::Queue;