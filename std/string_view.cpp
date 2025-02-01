#include <std/string_view.h>
#include <std/string.h>

namespace std {

StringView::StringView(const String& str) : m_data(str.data()), m_size(str.size()) {}

}