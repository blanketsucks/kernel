#if 0

#include <lai/host.h>
#include <lai/core.h>
#include <lai/helpers/sci.h>

#include <kernel/acpi/lai.h>
#include <kernel/serial.h>
#include <kernel/panic.h>
#include <kernel/memory/liballoc.h>
#include <kernel/memory/manager.h>
#include <kernel/arch/io.h>
#include <kernel/pci.h>
#include <kernel/acpi/acpi.h>

namespace kernel {

extern "C" {

void laihost_log(int level, const char* message) {
    if (level != LAI_DEBUG_LOG) {
        serial::printf("[LAI]: %s\n", message);
    }
}

__attribute__((noreturn)) void laihost_panic(const char* message) {
    panic(message);
}

void* laihost_malloc(size_t size) {
    return kmalloc(size);
}

void* laihost_realloc(void* ptr, size_t size, size_t) {
    return krealloc(ptr, size);
}

void laihost_free(void* ptr, size_t) {
    kfree(ptr);
}

void* laihost_map(size_t address, size_t size) {
    return MM->map_physical_region(reinterpret_cast<void*>(address), size);
}

void laihost_unmap(void*, size_t) {
    // TODO: Implement this
}

void* laihost_scan(const char* signature, size_t) {
    auto* parser = acpi::Parser::instance();
    return parser->find_table(signature);
}

void laihost_outb(u16 port, u8 value) {
    io::write<u8>(port, value);
}

void laihost_outw(u16 port, u16 value) {
    io::write<u16>(port, value);
}

void laihost_outd(u16 port, u32 value) {
    io::write<u32>(port, value);
}

u8 laihost_inb(u16 port) {
    return io::read<u8>(port);
}

u16 laihost_inw(u16 port) {
    return io::read<u16>(port);
}

u32 laihost_ind(u16 port) {
    return io::read<u32>(port);
}

u8 laihost_pci_readb(u16, u8 bus, u8 slot, u8 func, u16 offset) {
    pci::Address address = { bus, slot, func };
    return pci::read<u8>(address, offset);
}

u16 laihost_pci_readw(u16, u8 bus, u8 slot, u8 func, u16 offset) {
    pci::Address address = { bus, slot, func };
    return pci::read<u16>(address, offset);
}

u32 laihost_pci_readd(u16, u8 bus, u8 slot, u8 func, u16 offset) {
    pci::Address address = { bus, slot, func };
    return pci::read<u32>(address, offset);
}

void laihost_pci_writeb(u16, u8 bus, u8 slot, u8 func, u16 offset, u8 value) {
    pci::Address address = { bus, slot, func };
    pci::write<u8>(address, offset, value);
}

void laihost_pci_writew(u16, u8 bus, u8 slot, u8 func, u16 offset, u16 value) {
    pci::Address address = { bus, slot, func };
    pci::write<u16>(address, offset, value);
}

void laihost_pci_writed(u16, u8 bus, u8 slot, u8 func, u16 offset, u32 value) {
    pci::Address address = { bus, slot, func };
    pci::write<u32>(address, offset, value);
}

void laihost_sleep(u64) {
    // TODO: Implement this
}

}

}

namespace lai {

static constexpr char hex_to_ascii(u8 value) {
    return value < 10 ? '0' + value : 'A' + value - 10;
}

void eisaid_to_string(char* buffer, u32 eisaid) {
    u32 swapped = __builtin_bswap32(eisaid);

    buffer[0] = 0x40 + ((swapped >> 26) & 0x1F);
    buffer[1] = 0x40 + ((swapped >> 21) & 0x1F);
    buffer[2] = 0x40 + ((swapped >> 16) & 0x1F);

    buffer[3] = hex_to_ascii((swapped >> 12) & 0xF);
    buffer[4] = hex_to_ascii((swapped >> 8) & 0xF);
    buffer[5] = hex_to_ascii((swapped >> 4) & 0xF);
    buffer[6] = hex_to_ascii(swapped & 0xF);

    buffer[7] = '\0';
}

NodeIterator::NodeIterator(const lai_nsnode_t* node) {
    lai_initialize_ns_child_iterator(&m_iterator, const_cast<lai_nsnode_t*>(node));
    m_current = static_cast<Node*>(lai_ns_child_iterate(&m_iterator));
}

bool NodeIterator::operator==(const NodeIterator& other) const {
    return m_current == other.m_current;
}

Node& NodeIterator::operator*() const {
    return *m_current;
}

Node* NodeIterator::operator->() const {
    return m_current;
}

NodeIterator& NodeIterator::operator++() {
    m_current = static_cast<Node*>(lai_ns_child_iterate(&m_iterator));
    return *this;
}

NodeIterator NodeIterator::operator++(int) {
    NodeIterator copy = *this;
    ++(*this);

    return copy;
}

NodeIterator Node::begin() {
    return NodeIterator(this);
}

NodeIterator Node::end() {
    return NodeIterator();
}

Node* Node::root() {
    return static_cast<Node*>(lai_ns_get_root());
}

Node* Node::resolve(const char* path, Node* relative) {
    return static_cast<Node*>(lai_resolve_path(relative, path));
}

void Node::for_each_child(const Function<void(Node*)>& callback) {
    lai_nsnode_t* node = static_cast<lai_nsnode_t*>(this);

    lai_ns_child_iterator iterator;
    lai_initialize_ns_child_iterator(&iterator, node);

    lai_nsnode_t* child = nullptr;
    while ((child = lai_ns_child_iterate(&iterator))) {
        callback(static_cast<Node*>(child));
    }
}

char* Node::path() const {
    return lai_stringify_node_path(const_cast<lai_nsnode_t*>(static_cast<const lai_nsnode_t*>(this)));
}

template<> u64 Node::as<u64>() {
    if (lai_obj_get_type(&object) != LAI_INTEGER) {
        return 0;
    }

    u64 result = 0;
    lai_obj_get_integer(&object, &result);

    return result;
}

template<> char* Node::as<char*>() {
    if (lai_obj_get_type(&object) != LAI_STRING) {
        return nullptr;
    }

    return lai_exec_string_access(&object);
}

Status Node::evaluate_sta() {
    return { .value = static_cast<u32>(lai_evaluate_sta(this)) };
}

const char* node_type_to_string(int type) {
    switch (type) {
        case LAI_NAMESPACE_ROOT:
            return "Root";
        case LAI_NAMESPACE_NAME:
            return "Name";
        case LAI_NAMESPACE_ALIAS:
            return "Alias";
        case LAI_NAMESPACE_FIELD:
            return "Field";
        case LAI_NAMESPACE_METHOD:
            return "Method";
        case LAI_NAMESPACE_DEVICE:
            return "Device";
        case LAI_NAMESPACE_INDEXFIELD:
            return "IndexField";
        case LAI_NAMESPACE_MUTEX:
            return "Mutex";
        case LAI_NAMESPACE_PROCESSOR:
            return "Processor";
        case LAI_NAMESPACE_BUFFER_FIELD:
            return "BufferField";
        case LAI_NAMESPACE_THERMALZONE:
            return "ThermalZone";
        case LAI_NAMESPACE_EVENT:
            return "Event";
        case LAI_NAMESPACE_POWERRESOURCE:
            return "PowerResource";
        case LAI_NAMESPACE_BANKFIELD:
            return "BankField";
        case LAI_NAMESPACE_OPREGION:
            return "OpRegion";
        default:
            return "(Unknown)";
    }
}

const char* object_type_to_string(int type) {
    switch (type) {
        case LAI_INTEGER:
            return "Integer";
        case LAI_STRING:
            return "String";
        case LAI_BUFFER:
            return "Buffer";
        case LAI_PACKAGE:
            return "Package";
        case LAI_HANDLE:
            return "Handle";
        case LAI_LAZY_HANDLE:
            return "LazyHandle";
        case LAI_ARG_REF:
            return "ArgRef";
        case LAI_LOCAL_REF:
            return "LocalRef";
        case LAI_NODE_REF:
            return "NodeRef";
        case LAI_STRING_INDEX:
            return "StringIndex";
        case LAI_BUFFER_INDEX:
            return "BufferIndex";
        default:
            return "(Unknown)";
    }
}

void for_each_resource(lai_variable_t* variable, const Function<void(lai_resource_view*)>& callback) {
    if (variable->type != LAI_BUFFER) {
        return;
    }

    lai_resource_view view = LAI_RESOURCE_VIEW_INITIALIZER(variable);
    lai_resource_iterate(&view);

    while (lai_resource_get_type(&view) != LAI_RESOURCE_NULL) {
        callback(&view);
        lai_resource_iterate(&view);
    }
}

}

#endif