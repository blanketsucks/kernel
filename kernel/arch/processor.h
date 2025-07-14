#pragma once

#include <kernel/arch/tss.h>
#include <kernel/arch/cpu.h>

#include <std/string.h>

namespace kernel {

enum class InterruptState {
    Disabled = 0,
    Enabled = 1,
};

class Processor {
public:
    static void init();

    // For now there is only one instance but where we have SMP we will have multiple instances
    static Processor& instance();

    arch::TSS& tss() { return m_tss; }

    void initialize_context_switching(Thread* initial_thread);
    void switch_context(Thread* old, Thread* next);

    u32 id() const { return m_id; }

    arch::CPUFeatures features() const { return m_features; }
    bool has_feature(arch::CPUFeatures feature) const { return std::has_flag(m_features, feature); }

    u8 max_physical_address_width() const { return m_max_physical_address_width; }
    PhysicalAddress max_physical_address() const { return (1ull << m_max_physical_address_width) - 1; }

    u8 max_virtual_address_width() const { return m_max_virtual_address_width; }
    VirtualAddress max_virtual_address() const { return (1ull << m_max_virtual_address_width) - 1; }

    bool has_nx() const { return m_has_nx; }

    String const& brand() const { return m_brand; }

    String features_string() const;

    InterruptState interrupt_state() const;

private:
    void preinit();

    void* m_user_stack;
    arch::TSS m_tss;

    u32 m_id = 0;

    u8 m_max_physical_address_width;
    u8 m_max_virtual_address_width;
    bool m_has_nx = false;

    arch::CPUFeatures m_features;

    String m_brand;
};

}