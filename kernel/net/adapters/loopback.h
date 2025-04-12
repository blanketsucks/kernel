#pragma once

#include <kernel/net/adapter.h>

#include <std/memory.h>

namespace kernel::net {

class LoopbackAdapter : public NetworkAdapter {
public:
    static RefPtr<NetworkAdapter> create();

    Type type() const override { return Loopback; }

    void send(u8 const* data, size_t size) override;

private:
    LoopbackAdapter();
};

}