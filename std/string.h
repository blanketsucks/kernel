#pragma once

#include <std/traits.h>
#include <std/string_view.h>
#include <std/kmalloc.h>

namespace std {

template<typename T>
class Vector;

class String {
public:
    using Iterator = StringIterator;

    String() : m_data(nullptr), m_size(0), m_capacity(0) {}

    String(const char* str) : m_data(nullptr), m_size(0), m_capacity(0) {
        this->append(str);
    }

    String(const StringView& str) : m_data(nullptr), m_size(0), m_capacity(0) {
        this->append(str);
    }

    String(const String& other) : m_data(nullptr), m_size(0), m_capacity(0) {
        this->append(other);
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
    }

    void clear() {
        m_size = 0;
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

    String& operator=(const StringView& other);
    String& operator=(const String& other);
    String& operator=(const char* other);

    char operator[](size_t index) const { return m_data[index]; }

    void reserve(size_t capacity);
    void resize(size_t size);

    void append(char c);
    void append(const char* str);
    void append(const char* str, size_t len);
    void append(const StringView& str);
    void append(const String& str);

    char pop();

    StringView substr(size_t start, size_t end) const;
    StringView substr(size_t start) const;

    size_t find_last_of(char c) const { return this->rfind(c); }

    size_t find(char c, size_t start = 0) const;
    size_t find(const StringView& str, size_t start = 0) const;

    size_t rfind(char c, size_t start = StringView::npos) const;

    bool startswith(const StringView& str) const { return this->substr(0, str.size()) == str; }
    bool endswith(const StringView& str) const { return this->substr(m_size - str.size()) == str; }

    static String join(const Vector<String>& vec, char sep);

private:
    char* m_data;
    size_t m_size;
    size_t m_capacity;
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