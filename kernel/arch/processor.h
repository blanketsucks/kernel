#pragma once

#include <kernel/arch/tss.h>
#include <kernel/arch/cpu.h>
#include <kernel/arch/gdt.h>

#include <std/string.h>

namespace kernel {

enum class InterruptState {
    Disabled = 0,
    Enabled = 1,
};

class Processor {
public:
    void initialize(int cpu);

    static Processor& current();

    static bool are_interrupts_initialized();
    static void set_interrupts_initialized();

    static void enable_interrupts();
    static void disable_interrupts();

    [[gnu::naked]] static void syscall_interrupt_handler();

    arch::TSS& tss() { return m_tss; }

    [[noreturn]] void initialize_context_switching(Thread* initial_thread);
    [[noreturn]] void switch_to(Thread*);
    void switch_context(Thread* old, Thread* next);

    int cpu() const { return m_cpu; }

    arch::CPUFeatures features() const { return m_features; }
    bool has_feature(arch::CPUFeatures feature) const { return std::has_flag(m_features, feature); }

    u8 max_physical_address_width() const { return m_max_physical_address_width; }
    PhysicalAddress max_physical_address() const { return PhysicalAddress { (1ull << m_max_physical_address_width) - 1 }; }

    u8 max_virtual_address_width() const { return m_max_virtual_address_width; }
    VirtualAddress max_virtual_address() const { return VirtualAddress { (1ull << m_max_virtual_address_width) - 1 }; }

    bool has_nx() const { return m_has_nx; }

    String const& brand() const { return m_brand; }

    String features_string() const;

    InterruptState interrupt_state() const;

private:
    void preinit(int cpu);

    void write_gdt_entry(u16 selector, u32 low, u32 high);
    void write_gdt_entry(u16 selector, arch::GDTEntry const&);

    void write_tss();
    void init_gdt();

    int m_cpu;

    void* m_user_stack;
    arch::TSS m_tss;

    arch::GDTEntry m_gdt[256];
    u32 m_gdt_count;

    arch::GDTDescriptor m_gdtr;

    u8 m_max_physical_address_width;
    u8 m_max_virtual_address_width;

    bool m_has_nx = false;

    arch::CPUFeatures m_features;

    String m_brand;
};

}