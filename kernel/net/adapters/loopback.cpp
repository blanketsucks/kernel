#include <kernel/net/adapters/loopback.h>

namespace kernel::net {

RefPtr<NetworkAdapter> LoopbackAdapter::create() {
    return RefPtr<NetworkAdapter>(new LoopbackAdapter());
}

LoopbackAdapter::LoopbackAdapter() {
    set_mac_address({ 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 });

    set_ipv4_address({ 127, 0, 0, 1 });
    set_ipv4_netmask({ 255, 0, 0, 0 });
}

void LoopbackAdapter::send(u8 const* data, size_t size) {
    on_packet_receive(data, size);
}

}