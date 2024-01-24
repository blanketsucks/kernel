#pragma once

#include <kernel/common.h>
#include <std/utility.h>
#include <std/traits.h>

namespace std {

template<typename T>
class OwnPtr {
public:
    OwnPtr(const OwnPtr&) = delete;
    OwnPtr& operator=(const OwnPtr&) = delete;

    OwnPtr() : m_ptr(nullptr) {}
    OwnPtr(T* ptr) : m_ptr(ptr) {}
    OwnPtr(OwnPtr&& other) : m_ptr(other.m_ptr) { other.m_ptr = nullptr; }

    template<typename U>
    OwnPtr(OwnPtr<U>&& other) : m_ptr(static_cast<T*>(other.take())) {}

    ~OwnPtr() { if (m_ptr) delete m_ptr; }

    template<typename... Args>
    static inline OwnPtr make(Args&&... args) {
        return OwnPtr(new T(forward<Args>(args)...));
    }

    OwnPtr& operator=(OwnPtr&& other) {
        if (this == &other) {
            return *this;
        }

        delete m_ptr;

        m_ptr = other.m_ptr;
        other.m_ptr = nullptr;

        return *this;
    }

    T* operator->() { return m_ptr; }
    const T* operator->() const { return m_ptr; }

    T& operator*() { return *m_ptr; }
    const T& operator*() const { return *m_ptr; }

    T* ptr() { return m_ptr; }
    const T* ptr() const { return m_ptr; }

    operator bool() const { return m_ptr; }

    T* take() {
        T* ptr = m_ptr;
        m_ptr = nullptr;

        return ptr;
    }

private:
    T* m_ptr = nullptr;
};

class RefCount {
public:
    RefCount() : m_ref_count(0) {}

    void ref() { m_ref_count++; }
    void unref() { m_ref_count--; }

    u32 ref_count() const { return m_ref_count; }

private:
    u32 m_ref_count;
};

template<typename T>
class RefPtr {
public:
    RefPtr() = default;

    RefPtr(T* ptr) : m_ptr(ptr), m_ref_count(new RefCount) { m_ref_count->ref(); }
    RefPtr(const RefPtr& other) : m_ptr(other.m_ptr) {
        m_ref_count = other.m_ref_count;
        if (m_ref_count) {
            m_ref_count->ref();
        }
    }

    template<typename U>
    RefPtr(const RefPtr<U>& other) : m_ptr(static_cast<T*>(other.ptr())) {
        m_ref_count = other.ref_count();
        if (m_ref_count) {
            m_ref_count->ref();
        }
    }

    RefPtr(RefPtr&& other) : m_ptr(other.m_ptr), m_ref_count(other.m_ref_count) {
        other.m_ptr = nullptr;
        other.m_ref_count = nullptr;
    }

    template<typename... Args>
    static inline RefPtr make(Args&&... args) {
        return RefPtr(new T(std::forward<Args>(args)...));
    }

    ~RefPtr() {
        if (!m_ptr) return;

        m_ref_count->unref();
        if (m_ref_count->ref_count() == 0) {
            delete m_ptr;
            delete m_ref_count;
        }
    }

    RefPtr& operator=(const RefPtr& other) {
        if (this == &other) {
            return *this;
        }

        m_ptr = other.m_ptr;
        m_ref_count = other.m_ref_count;

        if (m_ref_count) m_ref_count->ref();
        return *this;
    }


    RefPtr& operator=(RefPtr&& other) {
        if (this == &other) {
            return *this;
        }

        m_ptr = other.m_ptr;
        m_ref_count = other.m_ref_count;

        other.m_ptr = nullptr;
        other.m_ref_count = nullptr;

        return *this;
    }

    T* operator->() { return m_ptr; }
    const T* operator->() const { return m_ptr; }

    T& operator*() { return *m_ptr; }
    const T& operator*() const { return *m_ptr; }

    T* ptr() { return m_ptr; }
    const T* ptr() const { return m_ptr; }

    operator bool() const { return m_ptr; }

    T* take() {
        T* ptr = m_ptr;
        m_ptr = nullptr;

        return ptr;
    }

    void ref() { if (m_ref_count) m_ref_count->ref(); }
    void unref() {if (m_ref_count) m_ref_count->unref(); }

    RefCount* ref_count() const {
        return m_ref_count;
    }

private:
    T* m_ptr = nullptr;
    RefCount* m_ref_count = nullptr;
};

}

namespace std::traits {

template<typename T>
struct Hash<OwnPtr<T>> {
    static size_t hash(const OwnPtr<T>& ptr) {
        return Hash<T*>::hash(ptr.ptr());
    }
};

template<typename T>
struct Hash<RefPtr<T>> {
    static size_t hash(const RefPtr<T>& ptr) {
        return Hash<T*>::hash(ptr.ptr());
    }
};

}

using std::OwnPtr;
using std::RefPtr;