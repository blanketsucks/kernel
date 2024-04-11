#pragma once

#include <std/types.h>
#include <std/kmalloc.h>
#include <std/initializer_list.h>

#include <kernel/serial.h>

namespace std {

template<typename T>
struct VectorIterator {
    VectorIterator(T* ptr, size_t offset = 0) : m_ptr(ptr), m_offset(offset) {}

    VectorIterator& operator++() {
        m_ptr++;
        m_offset++;

        return *this;
    }

    VectorIterator operator++(int) {
        VectorIterator iterator = *this;
        ++(*this);

        return iterator;
    }

    bool operator==(const VectorIterator& other) const { return m_ptr == other.m_ptr; }
    bool operator!=(const VectorIterator& other) const { return !(*this == other); }

    VectorIterator operator+(size_t offset) const { 
        return VectorIterator(m_ptr + offset, m_offset + offset);
    }

    VectorIterator operator-(size_t offset) const {
        return VectorIterator(m_ptr - offset, m_offset - offset);
    }

    VectorIterator operator+(const VectorIterator& other) const { 
        return VectorIterator(m_ptr + other.m_offset, m_offset + other.m_offset); 

    }
    VectorIterator operator-(const VectorIterator& other) const { 
        return VectorIterator(m_ptr - other.m_offset, m_offset - other.m_offset); 
    }

    T& operator*() { return *m_ptr; }
    const T& operator*() const { return *m_ptr; }

    size_t offset() const { return m_offset; }

private:
    T* m_ptr;
    size_t m_offset;
};

template<typename T>
class Vector {
public:
    using Iterator = VectorIterator<T>;
    using ConstIterator = VectorIterator<const T>;

    Vector() : m_size(0), m_capacity(0), m_data(nullptr) {}
    
    Vector(size_t size, const T& initial) : m_size(size), m_capacity(size) {
        this->reserve(m_capacity);
        for (size_t i = 0; i < size; i++) {
            m_data[i] = initial;
        }
    }

    Vector(const Vector<T>& other) {
        this->reserve(other.m_size);
        for (auto& item : other) {
            this->append(item);
        }
    }

    Vector(std::initializer_list<T> list) {
        this->reserve(list.size());
        for (auto& item : list) {
            this->append(item);
        }
    }

    Vector(Vector<T>&& other) : m_size(other.m_size), m_capacity(other.m_capacity), m_data(other.m_data) {
        other.m_data = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;
    }

    Vector& operator=(const Vector<T>& other) {
        if (this != &other) {
            this->clear();
            this->reserve(other.m_capacity);

            for (auto& item : other) {
                this->append(item);
            }
        }

        return *this;
    }

    Vector& operator=(Vector<T>&& other) {
        if (this != &other) {
            this->clear();
            this->reserve(other.m_capacity);

            for (auto& item : other) {
                this->append(item);
            }

            other.m_data = nullptr;
            other.m_size = 0;
            other.m_capacity = 0;
        }

        return *this;
    }

    ~Vector() {
        kfree(m_data);

        m_data = nullptr;
        m_size = 0;
    }

    void clear() {
        for (size_t i = 0; i < m_size; i++) {
            m_data[i].~T();
        }

        m_size = 0;
    }

    size_t size() const { return m_size; }
    size_t capacity() const { return m_capacity; }

    bool empty() const { return m_size == 0; }

    const T* data() const { return m_data; }
    T* data() { return m_data; }

    T& last() { return m_data[m_size - 1]; }
    const T& last() const { return m_data[m_size - 1]; }

    T& first() { return m_data[0]; }
    const T& first() const { return m_data[0]; }

    T take_first() {
        T item = m_data[0];
        this->remove(0);

        return item;
    }

    T take_last() {
        T item = m_data[m_size - 1];
        this->remove_last();

        return item;
    }

    // TODO: Maybe add bounds checking?
    T& operator[](size_t index) { return m_data[index]; }
    const T& operator[](size_t index) const { return m_data[index]; }

    Iterator begin() { return Iterator(m_data); }
    Iterator end() { return Iterator(m_data + m_size, m_size); }

    ConstIterator begin() const { return ConstIterator(m_data); }
    ConstIterator end() const { return ConstIterator(m_data + m_size, m_size); }

    void reserve(size_t capacity) {
        if (m_capacity >= capacity) {
            return;
        }

        T* data = reinterpret_cast<T*>(kmalloc(sizeof(T) * capacity));
        for (size_t i = 0; i < m_size; i++) {
            new (data + i) T(m_data[i]);
        }

        m_capacity = capacity;
        if (m_data) {
            for (size_t i = 0; i < m_size; i++) {
                m_data[i].~T();
            }

            kfree(m_data);
        }

        m_data = data;
    }

    void resize(size_t size) {
        this->reserve(size);
        m_size = size;
    }

    void shrink_to_fit() {
        if (m_capacity == m_size) {
            return;
        }

        this->reserve(m_size);
    }

    void append(const T& value) {
        if (m_size >= m_capacity) {
            this->reserve(m_capacity == 0 ? 1 : m_capacity * 2);
        }

        
        new (m_data + m_size) T(value);
        m_size++;
    }

    void append(const T* data, size_t size) {
        if (m_size + size >= m_capacity) {
            this->reserve(m_capacity == 0 ? size : m_capacity * 2);
        }

        for (size_t i = 0; i < size; i++) {
            m_data[m_size++] = data[i];
        }
    }

    void extend(const Vector<T>& vec) {
        for (auto& item : vec) {
            this->append(item);
        }
    }

    void remove(size_t index) {
        if (index >= m_size) {
            return;
        }

        m_data[index].~T();
        for (size_t i = index; i < m_size - 1; i++) {
            new (m_data + i) T(m_data[i + 1]);
            m_data[i + 1].~T();
        }

        m_size--;
    }

    void remove(const Iterator& it) {
        if (it == this->end()) {
            return;
        }

        auto offset = it.offset();
        this->remove(offset);
    }

    void remove(const T& value) {
        this->remove(this->find(value));
    }

    void remove_last() {
        if (m_size == 0) {
            return;
        }

        m_data[m_size - 1].~T();
        m_size--;
    }

    void reverse() {
        for (size_t i = 0; i < m_size / 2; i++) {
            T temp = m_data[i];

            m_data[i] = m_data[m_size - i - 1];
            m_data[m_size - i - 1] = temp;
        }
    }

    Iterator find(const T& value) {
        for (size_t i = 0; i < m_size; i++) {
            if (m_data[i] == value) {
                return Iterator(m_data + i, i);
            }
        }

        return this->end();
    }

    ConstIterator find(const T& value) const {
        for (size_t i = 0; i < m_size; i++) {
            if (m_data[i] == value) {
                return ConstIterator(m_data + i, i);
            }
        }

        return this->end();
    }

    bool contains(const T& value) const {
        return this->find(value) != this->end();
    }

private:    
    size_t m_size = 0;
    size_t m_capacity = 0;
    T* m_data = nullptr;
};

}

using std::Vector;