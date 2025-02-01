#pragma once

#include <kernel/common.h>

#include <std/bitmap.h>

namespace kernel::memory {

class BuddyAllocator {
public:
    static constexpr size_t MAX_ORDER = 12;

    BuddyAllocator(PhysicalAddress base, size_t pages);

    PhysicalAddress base_address() const { return m_base_address; }
    size_t total_pages() const { return m_total_pages; }

private:
    struct BuddyBucket {
        size_t index(i16 block) const {
            return (block >> order) / 2;
        }

        bool get(i16 block) {
            return bitmap.get(index(block));
        }

        void set(i16 block, bool value) {
            bitmap.set(index(block), value);
        }

        size_t order = 0;
        std::Bitmap bitmap;
        i16 freelist = -1;
    };

    PhysicalAddress m_base_address;
    size_t m_total_pages;

    BuddyBucket m_buckets[MAX_ORDER + 1];
};

}