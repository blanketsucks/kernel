#pragma once

#include <kernel/arch/tss.h>
#include <kernel/arch/cpu.h>
#include <kernel/arch/gdt.h>
#include <kernel/arch/cpuid.h>

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

    static InterruptState interrupt_state();
    static void set_interrupt_state(InterruptState);

    [[gnu::naked]] static void syscall_interrupt_handler();

    arch::TSS& tss() { return m_tss; }
    arch::GDTDescriptor const& gdtr() const { return m_gdtr; }

    [[noreturn]] void initialize_context_switching(Thread* initial_thread);
    [[noreturn]] void switch_to(Thread*);
    void switch_context(Thread* old, Thread* next);

    int cpu() const { return m_cpu; }

    arch::CPUFeatures const& features() const { return m_features; }
    bool has_feature(arch::Feature feature) const { return m_features.has(feature); }

    u8 max_physical_address_width() const { return m_max_physical_address_width; }
    PhysicalAddress max_physical_address() const { return PhysicalAddress { (1ull << m_max_physical_address_width) - 1 }; }

    u8 max_virtual_address_width() const { return m_max_virtual_address_width; }
    VirtualAddress max_virtual_address() const { return VirtualAddress { (1ull << m_max_virtual_address_width) - 1 }; }

    String const& brand() const { return m_brand; }

    String features_string() const;

    Thread* current_thread() const { return m_current_thread; }
    Thread* idle_thread() const { return m_idle_thread; }

    void set_idle_thread(Thread* thread) { m_idle_thread = thread; }

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

    arch::CPUFeatures m_features;

    String m_brand;

    Thread* m_current_thread = nullptr;
    Thread* m_idle_thread = nullptr;
};

}
