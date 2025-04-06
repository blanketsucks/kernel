#include <kernel/net/network_adapter.h>
#include <kernel/net/network_manager.h>
#include <kernel/net/ethernet.h>
#include <kernel/arch/interrupts.h>

namespace kernel::net {

void NetworkAdapter::on_packet_receive(u8 const* data, size_t size) {
    arch::InterruptDisabler disabler;

    u8* buffer = new u8[size];
    memcpy(buffer, data, size);

    Packet packet { buffer, size };
    m_packets.append(move(packet));

    NetworkManager::wakeup();
}

Packet NetworkAdapter::dequeue() {
    if (m_packets.empty()) {
        return { nullptr, 0 };
    }

    return m_packets.take_first();
}

void NetworkAdapter::send_packet(u8 const* data, size_t size) {
    this->send(data, size);
}

void NetworkAdapter::send(const MACAddress& destination, const ARPPacket& packet) {
    u8 buffer[sizeof(EthernetFrame) + sizeof(ARPPacket)];
    auto* frame = reinterpret_cast<EthernetFrame*>(buffer);
    
    frame->source = this->mac_address();
    frame->destination = destination;
    frame->type = EtherType::ARP;

    memcpy(&frame->payload[0], &packet, sizeof(ARPPacket));
    this->send(buffer, sizeof(buffer));
}

}