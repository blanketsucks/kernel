#pragma once

#include <std/traits.h>
#include <std/string_view.h>
#include <std/kmalloc.h>
#include <std/utility.h>

namespace std {

template<typename T>
class Vector;

class String {
public:
    using Iterator = StringIterator;

    String() = default;

    String(const char* str) {
        m_size = strlen(str);
        m_capacity = m_size;
        
        m_data = new char[m_capacity];
        memcpy(m_data, str, m_size);
    }

    String(const StringView& str) : m_size(str.size()), m_capacity(str.size()) {
        m_data = new char[m_size];
        memcpy(m_data, str.data(), m_size);
    }

    String(const String& other) : m_size(other.m_size), m_capacity(other.m_capacity) {
        m_data = new char[m_capacity];
        memcpy(m_data, other.m_data, m_size);
    }

    String(String&& other) : m_data(other.m_data), m_size(other.m_size), m_capacity(other.m_capacity) {
        other.m_data = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;
    }

    ~String() {
        if (m_data) {
            kfree(m_data);
        }

        m_data = nullptr;
    }

    void clear() {
        std::memset(m_data, 0, m_size);
        m_size = 0;
    }

    bool is_null_terminated() const {
        return m_data[m_size] == '\0';
    }

    const char* data() const { return m_data; }
    char* data() { return m_data; }

    size_t size() const { return m_size; }
    size_t capacity() const { return m_capacity; }

    bool empty() const { return m_size == 0; }

    char first() const { return m_data[0]; }
    char last() const { return m_data[m_size - 1]; }

    Iterator begin() const { return Iterator(m_data); }
    Iterator end() const { return Iterator(m_data + m_size); }

    bool operator==(const StringView& other) const;
    bool operator!=(const StringView& other) const;

    bool operator==(const char* other) const;
    bool operator!=(const char* other) const;

    bool operator==(const String& other) const;
    bool operator!=(const String& other) const;

    String& operator=(String&& other);
    String& operator=(const String& other);

    String& operator=(const StringView& other);
    String& operator=(const char* other);

    char operator[](size_t index) const { return m_data[index]; }

    void reserve(size_t capacity);
    void resize(size_t size);

    void grow(size_t new_capacity) {
        if (new_capacity <= m_capacity) {
            return;
        }

        this->reserve(max(new_capacity, m_capacity * 2));
    }

    void append(char c);
    void append(const char* str);
    void append(const char* str, size_t len);
    void append(const StringView& str);

    char pop();

    StringView substr(size_t start, size_t end) const;
    StringView substr(size_t start) const;

    size_t find_last_of(char c) const { return this->rfind(c); }

    size_t find(char c, size_t start = 0) const;
    size_t find(const StringView& str, size_t start = 0) const;

    size_t rfind(char c, size_t start = StringView::npos) const;

    bool startswith(const StringView& str) const { return this->substr(0, str.size()) == str; }
    bool startswith(char c) const { return m_size > 0 && m_data[0] == c; }

    bool endswith(const StringView& str) const { return this->substr(m_size - str.size()) == str; }
    bool endswith(char c) const { return m_size > 0 && m_data[m_size - 1] == c; }

    static String join(const Vector<String>& vec, char sep);
    static String format(const char* fmt, ...);

private:
    char* m_data = nullptr;
    
    size_t m_size = 0;
    size_t m_capacity = 0;
};

namespace traits {

template<>
struct Hash<String> {
    static size_t hash(const String& value) {
        size_t hash = 0;
        for (size_t i = 0; i < value.size(); i++) {
            hash += value[i];
        }

        return hash;
    }
};

}

}

using std::String;