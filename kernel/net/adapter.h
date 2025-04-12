#pragma once

#include <kernel/common.h>
#include <kernel/net/ip/ipv4.h>
#include <kernel/net/ip/arp.h>
#include <kernel/net/mac.h>

#include <std/vector.h>

namespace kernel::net {

struct Packet {
    u8* data;
    size_t size;
};

class NetworkAdapter {
public:
    enum Type {
        Loopback,
        Ethernet
    };

    virtual ~NetworkAdapter() = default;

    virtual Type type() const = 0;

    MACAddress const& mac_address() const { return m_mac_address; }

    IPv4Address const& ipv4_address() const { return m_ipv4_address; }
    IPv4Address const& ipv4_netmask() const { return m_ipv4_netmask; }

    void set_ipv4_address(IPv4Address const& address) { m_ipv4_address = address; }
    void set_ipv4_netmask(IPv4Address const& netmask) { m_ipv4_netmask = netmask; }

    void send_packet(u8 const* data, size_t size);
    void send(const MACAddress& destination, const ARPPacket& packet);

    Packet dequeue();

protected:
    virtual void send(u8 const* data, size_t size) = 0;

    void set_mac_address(MACAddress const& address) { m_mac_address = address; }

    void on_packet_receive(u8 const* data, size_t size);

private:
    MACAddress m_mac_address;

    IPv4Address m_ipv4_address;
    IPv4Address m_ipv4_netmask;

    Vector<Packet> m_packets; // FIXME: Use a proper queue
};

}