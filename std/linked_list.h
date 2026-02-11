/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <kernel/common.h>
#include <kernel/panic.h>

#include <std/utility.h>

namespace std {

template<typename L, typename E>
class LinkedListIterator {
public:
    using Node = typename L::Node;

    LinkedListIterator(Node* node, Node* prev = nullptr) : m_node(node), m_prev(prev) {}

    bool operator!=(const LinkedListIterator& other) const {
        return m_node != other.m_node;
    }

    bool operator==(const LinkedListIterator& other) const {
        return m_node == other.m_node;
    }

    LinkedListIterator& operator++() {
        m_prev = m_node;
        m_node = m_node->next;

        return *this;
    }

    E& operator*() { return m_node->value; }
    E* operator->() { return &m_node->value; }

    Node* node() const { return m_node; }

private:
    friend L;

    Node* m_node = nullptr;
    Node* m_prev = nullptr;
};

template<typename T>
class SinglyLinkedList {
public:
    using Iterator = LinkedListIterator<SinglyLinkedList, T>;
    using ConstIterator = LinkedListIterator<const SinglyLinkedList, const T>;

    SinglyLinkedList() = default;
    ~SinglyLinkedList() { this->clear(); }

    size_t size() const { return m_size; }
    bool empty() const { return !m_head; }

    void clear() {
        Node* node = m_head;
        while (node) {
            auto* next = node->next;
            delete node;

            node = next;
        }

        m_head = nullptr;
        m_tail = nullptr;
    }

    // FIXME: Add bounds checking
    T& first() { return m_head->value; }
    const T& first() const { return m_head->value; }

    T& last() { return m_tail->value; }
    const T& last() const { return m_tail->value; }

    T take_first() {
        auto* prev_head = m_head;
        T value = move(this->first());

        if (m_tail == m_head) {
            m_tail = nullptr;
        }

        m_head = m_head->next;
        delete prev_head;

        m_size--;
        return value;
    }

    void append(const T& value) {
        this->append(T(value));
    }

    void append(T&& value) {
        auto* node = new Node(move(value));
        m_size++;

        if (!m_head) {
            m_head = node;
            m_tail = node;

            return;
        }

        m_tail->next = node;
        m_tail = node;
    }

    void prepend(const T& value) {
        this->prepend(T(value));
    }

    void prepend(T&& value) {
        auto* node = new Node(move(value));
        m_size++;

        if (!m_head) {
            m_head = node;
            m_tail = node;

            return;
        }

        node->next = m_head;
        m_head = node;
    }

    bool contains(const T& value) const {
        for (auto* node = m_head; node; node = node->next) {
            if (node->value == value) {
                return true;
            }
        }

        return false;
    }

    Iterator begin() { return Iterator(m_head); }
    Iterator end() { return Iterator(nullptr); }

    ConstIterator begin() const { return ConstIterator(m_head); }
    ConstIterator end() const { return ConstIterator(nullptr); }

    template<typename F>
    ConstIterator find(F&& predicate) const {
        Node* prev = nullptr;
        for (auto* node = m_head; node; node = node->next) {
            if (predicate(node->value)) {
                return ConstIterator(node, prev);
            }
        
            prev = node;
        }

        return end();
    }

    template<typename F>
    Iterator find(F&& predicate) {
        Node* prev = nullptr;
        for (auto* node = m_head; node; node = node->next) {
            if (predicate(node->value)) {
                return Iterator(node, prev);
            }

            prev = node;
        }

        return end();
    }

    ConstIterator find(const T& value) const {
        return this->find([&](auto& other) { return value == other; });
    }

    Iterator find(const T& value) {
        return this->find([&](auto& other) { return value == other; });
    }

    void remove(Iterator iterator) {
        if (!iterator.m_node) {
            return;
        }

        if (iterator.m_node == m_head) {
            m_head = iterator.m_node->next;
        }

        if (iterator.m_node == m_tail) {
            m_tail = iterator.m_prev;
        }

        if (iterator.m_prev) {
            iterator.m_prev->next = iterator.m_node->next;
        }

        delete iterator.m_node;
    }

    void remove(const T& value) {
        auto it = this->find(value);
        if (it != this->end()) {
            this->remove(it);
        }
    }

private:
    friend Iterator;
    friend ConstIterator;

    struct Node {
        Node(T&& v) : value(move(v)) {}
        Node(const T& v) : value(v) {}

        T value;
        Node* next = nullptr;
    };

    Node* m_head = nullptr;
    Node* m_tail = nullptr;

    size_t m_size = 0;
};

template<typename L, typename E>
class ReverseLinkedListIterator {
public:
    using Node = typename L::Node;

    ReverseLinkedListIterator(Node* node) : m_node(node) {}

    bool operator!=(const ReverseLinkedListIterator& other) const {
        return m_node != other.m_node;
    }

    bool operator==(const ReverseLinkedListIterator& other) const {
        return m_node == other.m_node;
    }

    ReverseLinkedListIterator& operator++() {
        m_node = m_node->prev;
        return *this;
    }

    E& operator*() { return m_node->value; }
    E* operator->() { return &m_node->value; }

    Node* node() const { return m_node; }

private:
    friend L;

    Node* m_node = nullptr;
};

template<typename T>
class DoublyLinkedList {
public:
    using Iterator = LinkedListIterator<DoublyLinkedList, T>;
    using ConstIterator = LinkedListIterator<const DoublyLinkedList, const T>;

    using ReverseIterator = ReverseLinkedListIterator<DoublyLinkedList, T>;
    using ConstReverseIterator = ReverseLinkedListIterator<const DoublyLinkedList, const T>;

    DoublyLinkedList() = default;
    ~DoublyLinkedList() { this->clear(); }

    size_t size() const { return m_size; }
    bool empty() const { return !m_head; }

    void clear() {
        Node* node = m_head;
        while (node) {
            auto* next = node->next;
            delete node;

            node = next;
        }

        m_head = nullptr;
        m_tail = nullptr;
    }

    T& first() { return m_head->value; }
    const T& first() const { return m_head->value; }

    T& last() { return m_tail->value; }
    const T& last() const { return m_tail->value; }
    
    T take_first() {
        auto* prev_head = m_head;
        T value = move(this->first());

        if (m_tail == m_head) {
            m_tail = nullptr;
        }

        m_head = m_head->next;
        delete prev_head;

        return value;
    }

    T take_last() {
        auto* prev_tail = m_tail;
        T value = move(this->last());

        if (m_tail == m_head) {
            m_head = nullptr;
        }

        m_tail = m_tail->prev;
        delete prev_tail;

        return value;
    }

    void append(const T& value) {
        this->append(T(value));
    }

    void append(T&& value) {
        auto* node = new Node(move(value));
        m_size++;

        if (!m_head) {
            m_head = node;
            m_tail = node;

            return;
        }

        m_tail->next = node;
        node->prev = m_tail;

        m_tail = node;
    }

    void prepend(const T& value) {
        this->prepend(T(value));
    }

    void prepend(T&& value) {
        auto* node = new Node(move(value));
        m_size++;

        if (!m_head) {
            m_head = node;
            m_tail = node;

            return;
        }

        node->next = m_head;
        m_head->prev = node;

        m_head = node;
    }

    bool contains(const T& value) const {
        for (auto* node = m_head; node; node = node->next) {
            if (node->value == value) {
                return true;
            }
        }

        return false;
    }

    Iterator begin() { return Iterator(m_head); }
    Iterator end() { return Iterator(nullptr); }

    ConstIterator begin() const { return ConstIterator(m_head); }
    ConstIterator end() const { return ConstIterator(nullptr); }

    ReverseIterator rbegin() { return ReverseIterator(m_tail); }
    ReverseIterator rend() { return ReverseIterator(nullptr); }

    ConstReverseIterator rbegin() const { return ConstReverseIterator(m_tail); }
    ConstReverseIterator rend() const { return ConstReverseIterator(nullptr); }

    template<typename F>
    ConstIterator find(F&& predicate) const {
        Node* prev = nullptr;
        for (auto* node = m_head; node; node = node->next) {
            if (predicate(node->value)) {
                return ConstIterator(node, prev);
            }
        
            prev = node;
        }

        return end();
    }

    template<typename F>
    Iterator find(F&& predicate) {
        Node* prev = nullptr;
        for (auto* node = m_head; node; node = node->next) {
            if (predicate(node->value)) {
                return Iterator(node, prev);
            }

            prev = node;
        }

        return end();
    }

    ConstIterator find(const T& value) const {
        return this->find([&](auto& other) { return value == other; });
    }

    Iterator find(const T& value) {
        return this->find([&](auto& other) { return value == other; });
    }

    void remove(Iterator iterator) {
        if (!iterator.m_node) {
            return;
        }

        if (iterator.m_node == m_head) {
            m_head = iterator.m_node->next;
        }

        if (iterator.m_node == m_tail) {
            m_tail = iterator.m_prev;
        }

        if (iterator.m_prev) {
            iterator.m_prev->next = iterator.m_node->next;
        }

        if (iterator.m_node->next) {
            iterator.m_node->next->prev = iterator.m_prev;
        }

        delete iterator.m_node;
    }

    void remove(const T& value) {
        auto it = this->find(value);
        if (it != this->end()) {
            this->remove(it);
        }
    }

private:
    friend Iterator;
    friend ConstIterator;

    struct Node {
        Node(T&& v) : value(move(v)) {}
        Node(const T& v) : value(v) {}

        T value;

        Node* next = nullptr;
        Node* prev = nullptr;
    };

    Node* m_head = nullptr;
    Node* m_tail = nullptr;

    size_t m_size = 0;
};

}

using std::SinglyLinkedList;