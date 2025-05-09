#include <kernel/net/manager.h>

#include <kernel/net/ip/tcp.h>
#include <kernel/net/ip/udp.h>

#include <kernel/net/adapters/e1000.h>
#include <kernel/net/adapters/loopback.h>

#include <kernel/process/process.h>
#include <kernel/process/scheduler.h>

namespace kernel {

void handle_packet(net::NetworkAdapter& adapter, u8* data, size_t size);

void handle_arp_packet(net::NetworkAdapter& adapter, net::EthernetFrame* frame, size_t size);
void handle_ipv4_packet(net::NetworkAdapter& adapter, net::EthernetFrame* frame, size_t size);
void handle_ipv6_packet(net::NetworkAdapter& adapter, net::EthernetFrame* frame, size_t size);

void handle_tcp_packet(net::NetworkAdapter& adapter, net::IPv4Packet* packet, size_t size);
void handle_udp_packet(net::NetworkAdapter& adapter, net::IPv4Packet* packet, size_t size);
void handle_icmp_packet(net::NetworkAdapter& adapter, net::IPv4Packet* packet, size_t size);

using NetworkAdapterInitializer = RefPtr<net::NetworkAdapter> (*)(pci::Device);

static constexpr NetworkAdapterInitializer s_network_initializers[] = {
    net::E1000NetworkAdapter::create,
};

static NetworkManager s_instance = {};

NetworkManager* NetworkManager::instance() {
    return &s_instance;
}

RefPtr<net::NetworkAdapter> NetworkManager::create_network_adapter(pci::Device device) {
    for (auto& initializer : s_network_initializers) {
        auto adapter = initializer(device);
        if (adapter) {
            return adapter;
        }
    }

    return nullptr;
}

void NetworkManager::enumerate() {
    pci::enumerate([&](pci::Device device) {
        if (device.class_id() != pci::DeviceClass::NetworkController) {
            return;
        }

        auto adapter = this->create_network_adapter(device);
        if (!adapter) {
            return;
        }

        m_adapters.append(move(adapter));
    });
}

void NetworkManager::spawn() {
    auto* process = Process::create_kernel_process("Network Task", task);
    m_thread = process->get_main_thread();

    Scheduler::add_process(process);
}

void NetworkManager::initialize() {
    s_instance.enumerate();

    auto loopback = net::LoopbackAdapter::create();
    s_instance.m_loopback_adapter = loopback;

    s_instance.m_adapters.append(loopback);   

    s_instance.spawn();
}

void NetworkManager::wakeup() {
    s_instance.m_blocker.set_value(true);
}

void NetworkManager::add_adapter(RefPtr<net::NetworkAdapter> adapter) {
    m_adapters.append(move(adapter));
}

void NetworkManager::task() {
    s_instance.main();
}

void NetworkManager::main() {
    while (true) {
        m_blocker.set_value(false);
        m_blocker.wait();

        for (auto& adapter : m_adapters) {
            net::Packet packet = adapter->dequeue();
            while (packet.data) {
                handle_packet(*adapter, packet.data, packet.size);
                packet = adapter->dequeue();
            }
        }
    }
}
void handle_packet(net::NetworkAdapter& adapter, u8* data, size_t size) {
    auto* frame = reinterpret_cast<net::EthernetFrame*>(data);

if constexpr (false) {
    dbgln("Ethernet packet (size={}):", size);
    dbgln(" - Source: {}", frame->source);
    dbgln(" - Destination: {}", frame->destination);
    dbgln(" - Type: {}", frame->type);
}

    switch (frame->type) {
        case net::EtherType::ARP:
            handle_arp_packet(adapter, frame, size); break;
        case net::EtherType::IPv4:
            handle_ipv4_packet(adapter, frame, size); break;
        case net::EtherType::IPv6:
            handle_ipv6_packet(adapter, frame, size); break;
        default:
            dbgln("Unknown Ethernet type: {}", frame->type);
            break;
    }
}

void handle_arp_packet(net::NetworkAdapter& adapter, net::EthernetFrame* frame, size_t) {
    auto* arp = reinterpret_cast<net::ARPPacket*>(frame->payload);

    if (arp->operation == net::ARPOperation::Request) {
        net::ARPPacket response;

        response.sender_hardware_address = adapter.mac_address();
        response.sender_protocol_address = arp->target_protocol_address;

        response.target_hardware_address = arp->sender_hardware_address;
        response.target_protocol_address = arp->sender_protocol_address;

        response.operation = net::ARPOperation::Response;
        adapter.send(arp->sender_hardware_address, response);
    }
}

void handle_ipv4_packet(net::NetworkAdapter& adapter, net::EthernetFrame* frame, size_t size) {
    auto* ipv4 = reinterpret_cast<net::IPv4Packet*>(frame->payload);

if constexpr (false) {
    dbgln("IPv4 packet (size={}):", size);
    dbgln(" - Version: {}", ipv4->version);
    dbgln(" - IHL: {}", ipv4->ihl);
    dbgln(" - DSCP: {}", ipv4->dscp);
    dbgln(" - ECN: {}", ipv4->ecn);
    dbgln(" - Length: {}", ipv4->length);
    dbgln(" - Identification: {}", ipv4->identification);
    dbgln(" - Flags: {}", ipv4->flags_and_fragment_offset);
    dbgln(" - TTL: {}", ipv4->ttl);
    dbgln(" - Protocol: {}", ipv4->protocol);
    dbgln(" - Checksum: {}", ipv4->checksum);
    dbgln(" - Source: {}", ipv4->source);
    dbgln(" - Destination: {}", ipv4->destination);
}

    switch (ipv4->protocol) {
        case net::IPProtocol::TCP:
            handle_tcp_packet(adapter, ipv4, size); break;
        case net::IPProtocol::UDP:
            handle_udp_packet(adapter, ipv4, size); break;
        case net::IPProtocol::ICMP:
            handle_icmp_packet(adapter, ipv4, size); break;
        default:
            dbgln("Unknown IPv4 protocol: {}", ipv4->protocol);
            break;
    }
}

void handle_ipv6_packet(net::NetworkAdapter&, net::EthernetFrame*, size_t) {}

void handle_tcp_packet(net::NetworkAdapter&, net::IPv4Packet* packet, size_t size) {
    auto* tcp = reinterpret_cast<net::TCPPacket*>(packet);

    dbgln("TCP Packet (size={})", size);
    dbgln(" - Source port: {}", tcp->source_port);
    dbgln(" - Destination port: {}", tcp->destination_port);
    dbgln(" - Sequence number: {}", tcp->sequence_number);
    dbgln(" - Acknowledgment number: {}", tcp->ack_number);
    dbgln(" - Data offset: {}", tcp->data_offset & 0xF0);
    dbgln(" - Flags: {}", tcp->flags);
    dbgln(" - Window size: {}", tcp->window_size);
    dbgln(" - Checksum: {}", tcp->checksum);
    dbgln(" - Urgent pointer: {}", tcp->urgent_pointer);
}

void handle_udp_packet(net::NetworkAdapter&, net::IPv4Packet* packet, size_t size) {
    auto* udp = reinterpret_cast<net::UDPPacket*>(packet->payload);

    dbgln("UDP packet (size={}):", size);
    dbgln(" - Source port: {}", udp->source_port);
    dbgln(" - Destination port: {}", udp->destination_port);
    dbgln(" - Length: {}", udp->length);
    dbgln(" - Checksum: {}", udp->checksum);
}

void handle_icmp_packet(net::NetworkAdapter&, net::IPv4Packet*, size_t) {}

}