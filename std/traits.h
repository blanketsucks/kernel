#pragma once

#include <std/types.h>

namespace std::traits {

#define INT_TYPES(Op)   \
    Op(i8)              \
    Op(i16)             \
    Op(i32)             \
    Op(i64)             \
    Op(u8)              \
    Op(u16)             \
    Op(u32)             \
    Op(u64)             \

template<typename T>
struct Hash {
    static size_t hash(const T& value);
};

#define MAKE_HASH(Type)                                  \
    template<> struct Hash<Type> {                       \
        static size_t hash(Type value) { return value; } \
    };

INT_TYPES(MAKE_HASH)

template<typename T>
struct Hash<T*> {
    static size_t hash(T* value) {
        return reinterpret_cast<size_t>(value);
    }
};
    
}

#undef INT_TYPES
#undef MAKE_HASH