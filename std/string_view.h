#pragma once

#include <std/traits.h>
#include <std/cstring.h>

namespace std {

class StringIterator {
public:
    constexpr StringIterator(const char* ptr) : m_ptr(ptr) {}

    constexpr char operator*() const { return *m_ptr; }

    constexpr StringIterator& operator++() { m_ptr++; return *this; }
    constexpr StringIterator operator++(int) { StringIterator tmp = *this; ++(*this); return tmp; }

    constexpr bool operator==(const StringIterator& other) const { return m_ptr == other.m_ptr; }
    constexpr bool operator!=(const StringIterator& other) const { return !(*this == other); }

private:
    const char* m_ptr;
};

class String;

constexpr size_t __strlen(const char* str) {
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }

    return len;
}

class StringView {
public:
    using Iterator = StringIterator;

    static constexpr size_t npos = size_t(-1);

    constexpr StringView() : m_data(nullptr), m_size(0) {}
    constexpr StringView(const char* data, size_t size) : m_data(data), m_size(size) {}
    constexpr StringView(const char* data) : m_data(data), m_size(__strlen(data)) {}

    StringView(const String& str);

    constexpr const char* data() const { return m_data; }
    constexpr size_t size() const { return m_size; }

    constexpr bool is_null() const { return m_data == nullptr; }
    constexpr bool empty() const { return m_size == 0; }

    constexpr char first() const { return m_data[0]; }
    constexpr char last() const { return m_data[m_size - 1]; }

    constexpr Iterator begin() const { return Iterator(m_data); }
    constexpr Iterator end() const { return Iterator(m_data + m_size); }

    constexpr bool operator==(const StringView& other) const {
        if (this->size() != other.size()) {
            return false;
        }

        for (size_t i = 0; i < this->size(); i++) {
            if (this->data()[i] != other.data()[i]) {
                return false;
            }
        }

        return true;
    }

    constexpr bool operator!=(const StringView& other) const {
        return !(*this == other);
    }

    constexpr bool operator==(const char* other) const {
        return *this == StringView(other);
    }

    constexpr bool operator!=(const char* other) const {
        return !(*this == other);
    }

    constexpr char operator[](size_t index) const { return m_data[index]; }

    constexpr StringView substr(size_t start, size_t end) const {
        if (start >= m_size || end > m_size) {
            return {};
        }

        if (start > end) {
            return {};
        }

        return StringView(m_data + start, end - start);
    }

    constexpr StringView substr(size_t start) const {
        if (start >= m_size) {
            return {};
        }

        return StringView(m_data + start, m_size - start);
    }

    constexpr size_t find_last_of(char c) const {
        return this->rfind(c);
    }

    constexpr size_t find(char c, size_t start = 0) const {
        for (size_t i = start; i < m_size; i++) {
            if (m_data[i] == c) {
                return i;
            }
        }

        return npos;
    }

    constexpr size_t find(const StringView& str, size_t start = 0) const {
        for (size_t i = start; i < m_size; i++) {
            if (this->substr(i, i + str.size()) == str) {
                return i;
            }
        }

        return npos;
    }

    constexpr size_t rfind(char c, size_t start = npos) const {
        if (start == npos) {
            start = m_size - 1;
        }

        size_t index = start;
        while (true) {
            if (m_data[index] == c) {
                return index;
            } else if (!index) {
                break;
            }

            index--;
        }

        return npos;
    }

    constexpr size_t rfind(const StringView& str, size_t start = npos) {
        if (start == npos) {
            start = m_size - 1;
        }

        size_t index = start;
        while (true) {
            if (this->substr(index, index + str.size()) == str) {
                return index;
            } else if (!index) {
                break;
            }

            index--;
        }

        return npos;
    }

    constexpr bool startswith(const StringView& str) const {
        return this->substr(0, str.size()) == str;
    }

    constexpr bool startswith(char c) const {
        return m_data[0] == c;
    }

    constexpr bool endswith(const StringView& str) const {
        return this->substr(m_size - str.size(), m_size) == str;
    }

    constexpr bool endswith(char c) const {
        return m_data[m_size - 1] == c;
    }
    
private:
    const char* m_data = nullptr;
    size_t m_size = 0;
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