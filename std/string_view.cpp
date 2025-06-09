#include <std/string_view.h>
#include <std/string.h>
#include <std/vector.h>

namespace std {

StringView::StringView(const String& str) : m_data(str.data()), m_size(str.size()) {}

Vector<StringView> StringView::split(char delimiter) const {
    if (!m_size) {
        return {};
    }

    Vector<StringView> result;
    size_t start = 0;

    if (m_data[0] == delimiter) {
        start = 1;
    }

    for (size_t i = start; i < m_size; ++i) {
        if (m_data[i] == delimiter) {
            result.append({ m_data + start, i - start });
            start = i + 1;
        }
    }

    if (start < m_size) {
        result.append({ m_data + start, m_size - start });
    }

    return result;
}

}