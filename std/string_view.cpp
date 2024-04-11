#include <std/string_view.h>
#include <std/string.h>

namespace std {

StringView::StringView(const String& str) : m_data(str.data()), m_size(str.size()) {}

bool StringView::operator==(const StringView& other) const {
    if (this->size() != other.size()) {
        return false;
    }

    kernel::serial::printf("memcmp(%s, %s, %d)\n", this->data(), other.data(), this->size());
    return memcmp(this->data(), other.data(), this->size()) == 0;
}

bool StringView::operator!=(const StringView& other) const {
    return !(*this == other);
}

bool StringView::operator==(const char* other) const {
    return *this == StringView(other);
}

bool StringView::operator!=(const char* other) const {
    return !(*this == other);
}

StringView StringView::substr(size_t start, size_t end) const {
    return StringView(m_data + start, end - start);
}

StringView StringView::substr(size_t start) const {
    if (start >= m_size) return StringView();
    return StringView(m_data + start, m_size - start);
}

size_t StringView::find_last_of(char c) const {
    return this->rfind(c);
}

size_t StringView::find(char c, size_t start) const {
    for (size_t i = start; i < m_size; i++) {
        if (m_data[i] == c) {
            return i;
        }
    }

    return npos;
}

size_t StringView::find(const StringView& str, size_t start) const {
    for (size_t i = start; i < m_size; i++) {
        if (this->substr(i, i + str.size()) == str) {
            return i;
        }
    }

    return npos;
}

size_t StringView::rfind(char c, size_t start) const {
    if (start == npos) start = m_size - 1;

    size_t index = start;
    while (true) {
        if (m_data[index] == c) return index;

        if (!index) break;
        index--;
    }

    return npos;
}

size_t StringView::rfind(const StringView& str, size_t start) const {
    if (start == npos) start = m_size - 1;

    size_t index = start;
    while (true) {
        if (this->substr(index, index + str.size()) == str) return index;

        if (!index) break;
        index--;
    }

    return npos;
}

bool StringView::startswith(const StringView& str) const {
    return this->substr(0, str.size()) == str;
}

bool StringView::startswith(char c) const {
    return this->first() == c;
}

bool StringView::endswith(const StringView& str) const {
    return this->substr(m_size - str.size()) == str;
}

bool StringView::endswith(char c) const {
    return this->last() == c;
}

}