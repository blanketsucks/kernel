#pragma once

#include <lai/helpers/resource.h>
#include <lai/core.h>

#include <std/string_view.h>
#include <std/function.h>

namespace lai {

class Node;

union Status {
    u32 value;

    struct {
        u8 present : 1;    // Set if the device is present
        u8 enabled : 1;    // Set if the device is enabled and decoding its resources
        u8 visible : 1;    // Set if the device should be shown in the UI.
        u8 functional : 1; // Set if the device is functioning properly (cleared if device failed its diagnostics).
        u8 battery : 1;    // Set if the battery is present.

        u32 reserved : 27;
    };
};

void eisaid_to_string(char* buffer, u32 eisaid);

const char* node_type_to_string(int type);
const char* object_type_to_string(int type);

void for_each_resource(lai_variable_t* variable, const Function<void(lai_resource_view*)>& callback);

class Node;

class NodeIterator {
public:
    NodeIterator(const lai_nsnode_t* node);
    NodeIterator() = default;

    bool operator==(const NodeIterator& other) const;

    Node& operator*() const;
    Node* operator->() const;

    NodeIterator& operator++();
    NodeIterator operator++(int);

private:
    lai_ns_child_iterator m_iterator;
    Node* m_current = nullptr;
};

class Node : public lai_nsnode_t {
public:
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    static Node* root();

    static Node* resolve(const char* path, Node* relative = nullptr);

    void for_each_child(const Function<void(Node*)>& callback);

    Status evaluate_sta();

    template<typename T> T as() = delete;

    template<> u64 as<u64>();
    template<> char* as<char*>();

    char* path() const;

    NodeIterator begin();
    NodeIterator end();
};

}