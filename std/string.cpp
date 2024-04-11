#include <std/string.h>
#include <std/vector.h>

namespace std {

bool String::operator==(const StringView& other) const {
    if (this->size() != other.size()) {
        return false;
    }

    return memcmp(this->data(), other.data(), this->size()) == 0;
}

bool String::operator!=(const StringView& other) const {
    return !(*this == other);
}

bool String::operator==(const char* other) const { return *this == StringView(other); }
bool String::operator!=(const char* other) const { return !(*this == other); }

bool String::operator==(const String& other) const { return *this == StringView(other); }
bool String::operator!=(const String& other) const { return !(*this == other); }

String& String::operator=(const StringView& other) {
    this->clear();
    this->append(other);

    return *this;
}

String& String::operator=(const String& other) {
    this->clear();
    this->append(other);

    return *this;
}

String& String::operator=(String&& other) {
    if (this != &other) {
        if (m_data) {
            kfree(m_data);
        }

        m_data = other.m_data;
        m_size = other.m_size;
        m_capacity = other.m_capacity;

        other.m_data = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;
    }

    return *this;
}

String& String::operator=(const char* other) {
    this->clear();
    this->append(other);

    return *this;
}

void String::reserve(size_t capacity) {
    if (m_capacity >= capacity) {
        return;
    }

    if (m_data != nullptr) {
        m_data = reinterpret_cast<char*>(krealloc(m_data, capacity));
    } else {
        m_data = reinterpret_cast<char*>(kmalloc(capacity));
    }

    m_capacity = capacity;
}

void String::resize(size_t size) {
    if (m_size >= size) {
        m_size = size;
        return;
    }

    if (m_capacity < size) {
        this->reserve(size);
    }

    memset(m_data + m_size, 0, size - m_size);
    m_size = size;
}

void String::append(char c) {
    if (m_size + 1 >= m_capacity || !m_data) {
        this->reserve(m_capacity == 0 ? 1 : m_capacity * 2);
    }

    m_data[m_size++] = c;
}

void String::append(const char* str) {
    size_t len = strlen(str);    
    if (m_size + len >= m_capacity || !m_data) {
        this->reserve(m_capacity == 0 ? m_size + len : m_capacity * 2);
    }

    memcpy(m_data + m_size, str, len);
    m_size += len;
}

void String::append(const char* str, size_t len) {
    if (m_size + len >= m_capacity || !m_data) {
        this->reserve(m_capacity == 0 ? m_size + len : m_capacity * 2);
    }

    memcpy(m_data + m_size, str, len);
    m_size += len;
}

void String::append(const StringView& str) {
    if (m_size + str.size() >= m_capacity || !m_data) {
        this->reserve(m_capacity == 0 ? m_size + str.size() : m_capacity * 2);
    }

    memcpy(m_data + m_size, str.data(), str.size());
    m_size += str.size();
}

void String::append(const String& str) {
    if (m_size + str.size() >= m_capacity || !m_data) {
        this->reserve(m_capacity == 0 ? m_size + str.size() : m_capacity * 2);
    }

    memcpy(m_data + m_size, str.data(), str.size());
    m_size += str.size();
}

char String::pop() {
    if (m_size == 0) return 0;
    return m_data[--m_size];
}

StringView String::substr(size_t start, size_t end) const {
    return StringView(m_data + start, end - start);
}

StringView String::substr(size_t start) const {
    if (start >= m_size) return StringView();
    return StringView(m_data + start, m_size - start);
}

size_t String::find(char c, size_t start) const {
    for (size_t i = start; i < m_size; i++) {
        if (m_data[i] == c) {
            return i;
        }
    }

    return StringView::npos;
}

size_t String::find(const StringView& str, size_t start) const {
    for (size_t i = start; i < m_size; i++) {
        if (this->substr(i, i + str.size()) == str) {
            return i;
        }
    }

    return StringView::npos;
}

size_t String::rfind(char c, size_t start) const {
    if (start == StringView::npos) start = m_size - 1;

    size_t index = start;
    while (true) {
        if (m_data[index] == c) return index;

        if (!index) break;
        index--;
    }

    return StringView::npos;
}

String String::join(const Vector<String>& vec, char sep) {
    String result;
    for (size_t i = 0; i < vec.size(); i++) {
        result.append(vec[i]);
        if (i != vec.size() - 1) {
            result.append(sep);
        }
    }

    return result;

}

}