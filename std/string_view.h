#pragma once

#include <std/traits.h>
#include <std/cstring.h>

namespace std {

class StringIterator {
public:
    StringIterator(const char* ptr) : m_ptr(ptr) {}

    char operator*() const { return *m_ptr; }
    StringIterator& operator++() { m_ptr++; return *this; }
    StringIterator operator++(int) { StringIterator tmp = *this; ++(*this); return tmp; }

    bool operator==(const StringIterator& other) const { return m_ptr == other.m_ptr; }
    bool operator!=(const StringIterator& other) const { return !(*this == other); }

private:
    const char* m_ptr;
};

class String;

class StringView {
public:
    using Iterator = StringIterator;

    static constexpr size_t npos = size_t(-1);

    StringView() : m_data(nullptr), m_size(0) {}
    StringView(const char* data, size_t size) : m_data(data), m_size(size) {}
    StringView(const char* data) : m_data(data), m_size(std::strlen(data)) {}

    StringView(const String& str);

    const char* data() const { return m_data; }
    size_t size() const { return m_size; }

    bool is_null() const { return m_data == nullptr; }
    bool empty() const { return m_size == 0; }

    char first() const { return m_data[0]; }
    char last() const { return m_data[m_size - 1]; }

    Iterator begin() const { return Iterator(m_data); }
    Iterator end() const { return Iterator(m_data + m_size); }

    bool operator==(const StringView& other) const;
    bool operator!=(const StringView& other) const;

    bool operator==(const char* other) const;
    bool operator!=(const char* other) const;

    char operator[](size_t index) const { return m_data[index]; }

    StringView substr(size_t start, size_t end) const;
    StringView substr(size_t start) const;

    size_t find_last_of(char c) const;

    size_t find(char c, size_t start = 0) const;
    size_t find(const StringView& str, size_t start = 0) const;

    size_t rfind(char c, size_t start = npos) const;
    size_t rfind(const StringView& str, size_t start = npos) const;

    bool startswith(const StringView& str) const;
    bool startswith(char c) const;

    bool endswith(const StringView& str) const;
    bool endswith(char c) const;
    
private:
    const char* m_data;
    size_t m_size;
};

namespace traits {

template<>
struct Hash<StringView> {
    static size_t hash(const StringView& value) {
        size_t hash = 0;
        for (size_t i = 0; i < value.size(); i++) {
            hash += value[i];
        }

        return hash;
    }
};

}

}


using std::StringView;