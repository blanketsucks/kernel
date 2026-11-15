#pragma once

#include <std/types.h>
#include <std/type_traits.h>

namespace std {

namespace detail {

template<typename M> struct member_ptr_traits;

template<typename C, typename R>
struct member_ptr_traits<R C::*> {
    using Class  = C;
    using Member = R;
};

}

struct ListNode {
    ListNode* next = this;
    ListNode* prev = this;

    bool is_linked() const { return next != this; }

    void unlink() {
        prev->next = next;
        next->prev = prev;
        prev = next = this;
    }
};

template<auto Member>
class IntrusiveList {
public:
    using Traits = detail::member_ptr_traits<decltype(Member)>;
    using T      = Traits::Class;

    static_assert(std::is_same_v<typename Traits::Member, ListNode>, "member must be a ListNode");

    IntrusiveList() = default;

    IntrusiveList(const IntrusiveList&) = delete;
    IntrusiveList& operator=(const IntrusiveList&) = delete;

    static size_t node_offset() {
        ptrdiff_t offset = 0;
        auto m = Member;

        __builtin_memcpy(&offset, &m, sizeof(offset));
        return static_cast<size_t>(offset);
    }

    class Iterator {
    public:
        T& operator*() const  { return *owner_of(m_current); }
        T* operator->() const { return owner_of(m_current); }

        Iterator& operator++() {
            m_current = m_next;
            m_next = m_current->next;

            return *this;
        }

        bool operator!=(const Iterator& o) const { return m_current != o.m_current; }
        bool operator==(const Iterator& o) const { return m_current == o.m_current; }

    private:
        friend class IntrusiveList;
        explicit Iterator(ListNode* n) : m_current(n), m_next(n->next) {}

        ListNode* m_current;
        ListNode* m_next;
    };

    static ListNode* node_of(T& value) {
        return reinterpret_cast<ListNode*>(reinterpret_cast<u8*>(&value) + node_offset());
    }

    static T* owner_of(ListNode* node) {
        return reinterpret_cast<T*>(reinterpret_cast<u8*>(node) - node_offset());
    }

    bool empty() const { return !m_head.is_linked(); }

    void append(T& value) { insert(node_of(value), m_head.prev, &m_head); }
    void preppend(T& value) { insert(node_of(value), &m_head, m_head.next); }

    void insert_before(T& pos, T& value) {
        ListNode* current = node_of(pos);
        ListNode* node    = node_of(value);

        node->prev = current->prev;
        node->next = current;

        current->prev->next = node;
        current->prev = node;
    }

    Iterator begin() { return Iterator(m_head.next); }
    Iterator end() { return Iterator(&m_head); }

    T& first() { return *begin(); }

    static void remove(T& value) {
        auto* node = node_of(value);
        node->unlink();
    }

private:
    static void insert(ListNode* node, ListNode* prev, ListNode* next) {
        node->next = next;
        node->prev = prev;

        prev->next = node;
        next->prev = node;
    }

    ListNode m_head;
};

}
