#pragma once

#include <kernel/common.h>
#include <kernel/net/adapter.h>
#include <kernel/process/threads.h>
#include <kernel/process/blocker.h>
#include <kernel/pci.h>

#include <std/vector.h>
#include <std/memory.h>

namespace kernel {

class NetworkManager {
public:
    using AdapterList = Vector<RefPtr<net::NetworkAdapter>>;

    static void initialize();

    static NetworkManager* instance();

    AdapterList const& adapters() const { return m_adapters; }

    static void wakeup();

    void add_adapter(RefPtr<net::NetworkAdapter> adapter);

private:
    RefPtr<net::NetworkAdapter> create_network_adapter(pci::Device);

    void enumerate();
    void spawn();

    void main();
    static void task();

    Vector<RefPtr<net::NetworkAdapter>> m_adapters;
    RefPtr<net::NetworkAdapter> m_loopback_adapter;

    Thread* m_thread = nullptr;
    BooleanBlocker m_blocker;
};

}