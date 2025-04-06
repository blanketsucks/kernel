/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <std/linked_list.h>
#include <std/traits.h>
#include <std/utility.h>
#include <std/initializer_list.h>

namespace std {

template<typename T, typename = traits::Hash<T>>
class HashTable;

template<typename HashTableType, typename ElementType, typename BucketIteratorType>
class HashTableIterator {
public:
    bool operator!=(const HashTableIterator& other) const {
        if (m_is_end && other.m_is_end) {
            return false;
        }

        return m_table != other.m_table || m_is_end != other.m_is_end 
                || m_bucket_index != other.m_bucket_index 
                || m_bucket_iterator != other.m_bucket_iterator;
    }

    bool operator==(const HashTableIterator& other) const { return !(*this != other); }

    ElementType& operator*() { return *m_bucket_iterator; }
    ElementType* operator->() { return m_bucket_iterator.operator->(); }

    HashTableIterator& operator++() {
        this->skip_to_next();
        return *this;
    }

    void skip_to_next() {
        while (!m_is_end) {
            if (m_bucket_iterator.is_end()) {
                ++m_bucket_index;
                if (m_bucket_index >= m_table->capacity()) {
                    m_is_end = true;
                    return;
                }
                m_bucket_iterator = m_table->bucket(m_bucket_index).begin();
            } else {
                ++m_bucket_iterator;
            }

            if (!m_bucket_iterator.is_end()) {
                return;
            }
        }
    }

private:
    friend HashTableType;

    explicit HashTableIterator(
        HashTableType* table, 
        bool is_end, 
        BucketIteratorType bucket_iterator = BucketIteratorType::universal_end(), 
        size_t bucket_index = 0
    ) : m_table(table), m_bucket_index(bucket_index), m_is_end(is_end), m_bucket_iterator(bucket_iterator) {

        bool is_universal_end = m_bucket_iterator == BucketIteratorType::universal_end();
        if (!is_end && !m_table->empty() && is_universal_end) {
            m_bucket_iterator = m_table->bucket(0).begin();
            if (m_bucket_iterator.is_end())
                this->skip_to_next();
        }
    }

    HashTableType* m_table;
    size_t m_bucket_index = 0;
    bool m_is_end = false;
    BucketIteratorType m_bucket_iterator;
};

template<typename T, typename Hash>
class HashTable {
private:
    using Bucket = SinglyLinkedList<T>;

public:
    using ConstIterator = HashTableIterator<const HashTable, const T, typename Bucket::ConstIterator>;
    using Iterator = HashTableIterator<HashTable, T, typename Bucket::Iterator>;

    HashTable() = default;

    HashTable(std::initializer_list<T> list) {
        this->reserve(list.size());
        for (auto& value : list) {
            this->set(value);
        }
    }

    HashTable(const HashTable& other) {
        this->reserve(other.size());
        for (auto& it : other)
            this->set(it);
    }

    HashTable& operator=(const HashTable& other) {
        if (this == &other) {
            return *this;
        }

        this->clear();
        this->reserve(other.size());
        for (auto& entry : other) {
            this->set(entry);
        }

        return *this;
    }

    HashTable(HashTable&& other) : m_buckets(other.m_buckets), m_size(other.m_size), m_capacity(other.m_capacity) {
        other.m_size = 0;
        other.m_capacity = 0;
        other.m_buckets = nullptr;
    }

    HashTable& operator=(HashTable&& other) {
        if (this == &other) {
            return *this;
        }

        this->clear();

        m_buckets = other.m_buckets;
        m_size = other.m_size;
        m_capacity = other.m_capacity;

        other.m_size = 0;
        other.m_capacity = 0;
        other.m_buckets = nullptr;

        return *this;
    }

    ~HashTable() { this->clear(); }

    bool empty() const { return m_size == 0; }
    size_t size() const { return m_size; }
    size_t capacity() const { return m_capacity; }

    void reserve(size_t capacity) {
        if (capacity <= m_capacity) {
            return;
        }

        this->rehash(capacity);
    }

    Iterator begin() { return Iterator(this, this->empty()); }
    Iterator end() { return Iterator(this, true); }

    ConstIterator begin() const { return ConstIterator(this, this->empty()); }
    ConstIterator end() const { return ConstIterator(this, true); }

    template<typename Finder>
    Iterator find(size_t hash, Finder finder) {
        if (this->empty()) return this->end();
        
        size_t index = 0;
        auto& bucket = this->lookup_with_hash(hash, &index);

        auto iterator = bucket.find(finder);
        if (iterator != bucket.end()) {
            return Iterator(this, false, iterator, index);
        }

        return this->end();
    }

    template<typename Finder>
    ConstIterator find(unsigned hash, Finder finder) const {
        if (this->empty()) return this->end();

        size_t index = 0;
        auto& bucket = this->lookup_with_hash(hash, &index);

        auto iterator = bucket.find(finder);
        if (iterator != bucket.end()) {
            return ConstIterator(this, false, iterator, index);
        }

        return this->end();
    }

    Iterator find(const T& value) {
        return this->find(Hash::hash(value), [&](auto& other) { return value == other; });
    }

    ConstIterator find(const T& value) const {
        return this->find(Hash::hash(value), [&](auto& other) { return value == other; });
    }

    void remove(const T& value) {
        auto it = this->find(value);
        if (it != this->end()) {
            this->remove(it);
        }
    }

    void remove(Iterator);
    void set(const T&);
    void set(T&&);
    bool contains(const T&) const;
    void clear();

private:
    friend Iterator;
    friend ConstIterator;

    Bucket& lookup(const T&, size_t* bucket_index = nullptr);
    const Bucket& lookup(const T&, size_t* bucket_index = nullptr) const;

    Bucket& lookup_with_hash(size_t hash, size_t* bucket_index) {
        if (bucket_index)
            *bucket_index = hash % m_capacity;
        return m_buckets[hash % m_capacity];
    }

    const Bucket& lookup_with_hash(size_t hash, size_t* bucket_index) const {
        if (bucket_index)
            *bucket_index = hash % m_capacity;
        return m_buckets[hash % m_capacity];
    }

    void rehash(size_t capacity);
    void insert(const T&);
    void insert(T&&);

    Bucket& bucket(size_t index) { return m_buckets[index]; }
    const Bucket& bucket(size_t index) const { return m_buckets[index]; }

    Bucket* m_buckets = nullptr;

    size_t m_size = 0;
    size_t m_capacity = 0;
};

template<typename T, typename Hash>
void HashTable<T, Hash>::set(T&& value) {
    if (!m_capacity) this->rehash(1);

    auto& bucket = this->lookup(value);
    for (auto& entry : bucket) {
        if (entry == value) {
            entry = std::move(value);
            return;
        }
    }

    if (m_size >= m_capacity) {
        this->rehash(m_size + 1);
        this->insert(std::move(value));
    } else {
        bucket.append(std::move(value));
    }

    m_size++;
}

template<typename T, typename Hash>
void HashTable<T, Hash>::set(const T& value) {
    if (!m_capacity) this->rehash(1);

    auto& bucket = this->lookup(value);
    for (auto& entry : bucket) {
        if (entry == value) {
            entry = value;
            return;
        }
    }

    if (m_size >= m_capacity) {
        this->rehash(m_size + 1);
        this->insert(value);
    } else {
        bucket.append(value);
    }
    
    m_size++;
}

template<typename T, typename Hash>
void HashTable<T, Hash>::rehash(size_t new_capacity) {
    new_capacity *= 2;

    auto* new_buckets = new Bucket[new_capacity];
    auto* old_buckets = m_buckets;
    size_t old_capacity = m_capacity;

    m_buckets = new_buckets;
    m_capacity = new_capacity;

    for (size_t i = 0; i < old_capacity; ++i) {
        for (auto& value : old_buckets[i]) {
            this->insert(std::move(value));
        }
    }

    delete[] old_buckets;
}

template<typename T, typename Hash>
void HashTable<T, Hash>::clear() {
    if (m_buckets) {
        delete[] m_buckets;
        m_buckets = nullptr;
    }

    m_capacity = 0;
    m_size = 0;
}

template<typename T, typename Hash>
void HashTable<T, Hash>::insert(T&& value) {
    auto& bucket = this->lookup(value);
    bucket.append(move(value));
}

template<typename T, typename Hash>
void HashTable<T, Hash>::insert(const T& value) {
    auto& bucket = this->lookup(value);
    bucket.append(value);
}

template<typename T, typename Hash>
bool HashTable<T, Hash>::contains(const T& value) const {
    if (this->empty()) return false;

    auto& bucket = this->lookup(value);
    for (auto& entry : bucket) {
        if (entry == value) {
            return true;
        }
    }

    return false;
}

template<typename T, typename Hash>
void HashTable<T, Hash>::remove(Iterator it) {
    if (this->empty()) return;

    m_buckets[it.m_bucket_index].remove(it.m_bucket_iterator);
    --m_size;
}

template<typename T, typename Hash>
auto HashTable<T, Hash>::lookup(const T& value, size_t* bucket_index) -> Bucket& {
    size_t hash = Hash::hash(value);
    if (bucket_index)
        *bucket_index = hash % m_capacity;

    return m_buckets[hash % m_capacity];
}

template<typename T, typename Hash>
auto HashTable<T, Hash>::lookup(const T& value, size_t* bucket_index) const -> const Bucket& {
    size_t hash = Hash::hash(value);
    if (bucket_index)
        *bucket_index = hash % m_capacity;
    return m_buckets[hash % m_capacity];
}

}

using std::HashTable;