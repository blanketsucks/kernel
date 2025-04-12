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

#include <std/hash_table.h>
#include <std/traits.h>
#include <std/optional.h>
#include <std/initializer_list.h>

namespace std {

template<typename K, typename V>
struct HashMapEntry {
    K key;
    V value;

    bool operator==(const HashMapEntry& other) const { return key == other.key; }
    bool operator==(const K& key) const { return this->key == key; }
};

namespace traits {

template<typename K, typename V>
struct Hash<HashMapEntry<K, V>> {
    static size_t hash(const HashMapEntry<K, V>& entry) { return Hash<K>::hash(entry.key); }
};

}

template<typename K, typename V>
class HashMap {
public:
    using Hash = traits::Hash<K>;
    using TableType = HashTable<HashMapEntry<K, V>>;

    using Iterator = typename TableType::Iterator;
    using ConstIterator = typename TableType::ConstIterator;

    HashMap() = default;

    HashMap(std::initializer_list<HashMapEntry<K, V>> list) : m_table(list) {}

    bool empty() const { return m_table.empty(); }
    size_t size() const { return m_table.size(); }
    size_t capacity() const { return m_table.capacity(); }

    void clear() { m_table.clear(); }

    void set(const K& key, const V& value) { m_table.set({ key, value }); }
    void set(const K& key, V&& value) { m_table.set({ key, move(value) }); }

    void remove(const K& key) {
        auto iterator = this->find(key);
        if (iterator != this->end()) {
            m_table.remove(iterator);
        }
    }

    Iterator begin() { return m_table.begin(); }
    Iterator end() { return m_table.end(); }

    Iterator find(const K& key) {
        return m_table.find(Hash::hash(key), [&](auto& entry) { return key == entry.key; });
    }

    template<typename F>
    Iterator find(unsigned hash, F&& finder) {
        return m_table.find(hash, finder);
    }

    ConstIterator begin() const { return m_table.begin(); }
    ConstIterator end() const { return m_table.end(); }

    ConstIterator find(const K& key) const {
        return m_table.find(Hash::hash(key), [&](auto& entry) { return key == entry.key; });
    }

    template<typename F>
    ConstIterator find(size_t hash, F&& finder) const {
        return m_table.find(hash, finder);
    }

    void reserve(size_t capacity) { m_table.reserve(capacity); }

    bool contains(const K& key) const {
        return this->find(key) != this->end();
    }

    void remove(Iterator it) {
        m_table.remove(it);
    }

    Optional<V> get(const K& key) const {
        auto iterator = this->find(key);
        if (iterator == this->end())
            return {};

        return iterator->value;
    }

    V& ensure(const K& key) {
        auto iterator = this->find(key);
        if (iterator != this->end()) {
            return iterator->value;
        }

        this->set(key, V());
        return this->find(key)->value;
    }

private:
    TableType m_table;
};

}

using std::HashMap;