#pragma once

namespace std {

class SourceLocation {
public:
    constexpr SourceLocation(
        const char* file_name, const char* function_name,
        int line, int column
    ) : m_file_name(file_name), m_function_name(function_name), m_line(line), m_column(column) {}

    static constexpr SourceLocation current(
        const char* file_name = __builtin_FILE(),
        const char* function_name = __builtin_FUNCTION(),
        int line = __builtin_LINE(),
        int column = __builtin_COLUMN()
    ) {
        return SourceLocation(file_name, function_name, line, column);
    }

    constexpr const char* file_name() const { return m_file_name; }
    constexpr const char* function_name() const { return m_function_name; }
    constexpr int line() const { return m_line; }
    constexpr int column() const { return m_column; }

private:
    const char* m_file_name;
    const char* m_function_name;
    int m_line;
    int m_column;
};


}