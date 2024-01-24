#pragma once

#include <kernel/common.h>

namespace std {

class Bitmap {
public:
    Bitmap() = default;

    Bitmap(u8* data, size_t size) : m_data(data), m_size(size) {}

    size_t size() const { return m_size; }

    u8* data() { return m_data; }
    const u8* data() const { return m_data; }

    bool get(size_t index) const {
        if (index >= m_size) return false;
        return m_data[index / 8] & (1 << (index % 8));
    }

    void set(size_t index, bool value) {
        if (index >= m_size) return;
        if (value) {
            m_data[index / 8] |= (1 << (index % 8));
        } else {
            m_data[index / 8] &= ~(1 << (index % 8));
        }
    }

    size_t find_first_set() const {
        for (size_t i = 0; i < m_size; ++i) {
            if (this->get(i)) {
                return i;
            }
        }

        return m_size;
    }

    size_t find_first_unset() const {
        for (size_t i = 0; i < m_size; ++i) {
            if (!this->get(i)) {
                return i;
            }
        }

        return m_size;
    }

private:
    u8* m_data = nullptr;
    size_t m_size = 0;
};

}
