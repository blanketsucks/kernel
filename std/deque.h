#pragma once

#include <std/linked_list.h>

namespace std {

template<typename T>
class Deque {
public:
    using Iterator = typename DoublyLinkedList<T>::Iterator;
    using ConstIterator = typename DoublyLinkedList<T>::ConstIterator;

    Deque() = default;
    ~Deque() { this->clear(); }

    size_t size() const { return m_list.size(); }
    bool empty() const { return m_list.empty(); }

    void clear() { m_list.clear(); }

    void push_front(const T& value) {
        m_list.prepend(value);
    }

    void push_front(T&& value) {
        m_list.prepend(move(value));
    }

    void push_back(const T& value) {
        m_list.append(value);
    }

    void push_back(T&& value) {
        m_list.append(move(value));
    }

    T pop_front() { return m_list.take_first(); }
    T pop_back() { return m_list.take_last(); }

    T& front() { return m_list.first(); }
    const T& front() const { return m_list.first(); }

    T& back() { return m_list.last(); }
    const T& back() const { return m_list.last(); }

    Iterator begin() { return m_list.begin(); }
    Iterator end() { return m_list.end(); }

    ConstIterator begin() const { return m_list.begin(); }
    ConstIterator end() const { return m_list.end(); }

    template<typename F>
    ConstIterator find(F&& predicate) const {
        return m_list.find(predicate);
    }

    template<typename F>
    Iterator find(F&& predicate) {
        return m_list.find(predicate);
    }

    ConstIterator find(const T& value) const {
        return m_list.find(value);
    }

    Iterator find(const T& value) {
        return m_list.find(value);
    }

    void remove(Iterator it) { m_list.remove(it); }
    void remove(const T& value) { m_list.remove(value);}

    bool contains(const T& value) const { return m_list.contains(value); }
    bool contains(const T& value) { return m_list.contains(value); }

private:
    DoublyLinkedList<T> m_list;
};

}