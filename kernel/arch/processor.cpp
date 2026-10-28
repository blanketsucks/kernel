#include <kernel/arch/processor.h>
#include <kernel/arch/registers.h>
#include <kernel/process/threads.h>
#include <kernel/process/process.h>
#include <kernel/arch/cpu.h>
#include <kernel/arch/idt.h>
#include <kernel/arch/gdt.h>

namespace kernel {

extern "C" void _switch_context(arch::ThreadRegisters*, arch::ThreadRegisters*);
extern "C" [[noreturn]] void _switch_context_no_state(arch::ThreadRegisters*);

static bool s_interrupts_initialized = false;

Processor& Processor::current() {
    return *reinterpret_cast<Processor*>(arch::read_gs());
}

bool Processor::are_interrupts_initialized() {
    return s_interrupts_initialized;
}

void Processor::set_interrupts_initialized() {
    s_interrupts_initialized = true;
}

void Processor::enable_interrupts() {
    if (!s_interrupts_initialized) {
        return;
    }

    asm volatile("sti");
}

void Processor::disable_interrupts() {
    if (!s_interrupts_initialized) {
        return;
    }

    asm volatile("cli");
}

void Processor::preinit(int cpu) {
    m_cpu = cpu;
    m_features = arch::cpu_features();

    u32 eax, ebx, ecx, edx;
    arch::cpuid(0x80000000, eax, ebx, ecx, edx);

    u32 max_extended_leaf = eax;
    if (max_extended_leaf >= 0x80000001) {
        arch::cpuid(0x80000001, eax, ebx, ecx, edx);
        m_has_nx = (edx & (1 << 20));
    } else {
        m_has_nx = false;
    }

    if (max_extended_leaf >= 0x80000004) {
        char brand[48];
        for (int i = 0; i < 3; i++) {
            arch::cpuid(0x80000002 + i, eax, ebx, ecx, edx);

            memcpy(brand + i * 16, &eax, 4);
            memcpy(brand + i * 16 + 4, &ebx, 4);
            memcpy(brand + i * 16 + 8, &ecx, 4);
            memcpy(brand + i * 16 + 12, &edx, 4);
        }

        brand[47] = '\0';
        m_brand = brand;
    }

    if (max_extended_leaf >= 0x80000008) {
        arch::cpuid(0x80000008, eax, ebx, ecx, edx);

        m_max_physical_address_width = eax & 0xFF;
        m_max_virtual_address_width = (eax >> 8) & 0xFF;
    } else {
        m_max_physical_address_width = has_feature(arch::CPUFeatures::PAE) ? 36 : 32;
        m_max_virtual_address_width = 32;
    }

    dbgln("Processor #{}:", m_cpu);
    dbgln(" - Brand: {}", m_brand);
    dbgln(" - Max Physical Address Width: {} bits", m_max_physical_address_width);
    dbgln(" - Max Virtual Address Width: {} bits", m_max_virtual_address_width);
    dbgln(" - Features: {}", features_string());

    dbgln();
}

void Processor::initialize(int cpu) {
    this->preinit(cpu);

    arch::init_gdt(m_tss);
    arch::init_idt();

    if (has_feature(arch::CPUFeatures::SSE)) {
        arch::write_cr0((arch::read_cr0() & 0xfffffffb) | 2); // Clear CR0.EM and set CR0.MP
        arch::write_cr4(arch::read_cr4() | 0x600);            // Set CR4.OSFXSR and CR4.OSXMMEXCPT
    }

    u64 efer = arch::rmsr(arch::MSR_EFER);
    if (has_nx()) {
        efer |= (1 << 11);
    }

    // Enable and setup the `syscall` instruction
    arch::wmsr(arch::MSR_EFER, efer | 1);
    arch::wmsr(arch::MSR_STAR, (0x28ul << 32) | (0x33ul << 48));
    arch::wmsr(arch::MSR_LSTAR, reinterpret_cast<u64>(&Processor::syscall_interrupt_handler));

    // TODO: Setup swapgs
    arch::write_gs(reinterpret_cast<FlatPtr>(this));
}

void Processor::initialize_context_switching(Thread* initial_thread) {
    m_tss.init(initial_thread);
    _switch_context_no_state(&initial_thread->registers());
}

void Processor::switch_to(Thread* thread) {
    auto& kernel_stack = thread->kernel_stack();
    m_tss.set_kernel_stack(kernel_stack.top());

    _switch_context_no_state(&thread->registers());
}

void Processor::switch_context(Thread* old, Thread* next) {
    auto& kernel_stack = next->kernel_stack();
    m_tss.set_kernel_stack(kernel_stack.top());

    _switch_context(&old->registers(), &next->registers());
}

String Processor::features_string() const {
    String features;
#define Op(name, str, _)                              \
    if (has_feature(arch::CPUFeatures::name)) {      \
        features.append(str);                        \
        features.append(',');                        \
    }
    ENUMERATE_CPU_FEATURES(Op)
#undef Op

    if (features.size() > 0) {
        features.pop();
    }

    return features;
}

InterruptState Processor::interrupt_state() const {
    arch::Flags flags(arch::cpu_flags());
    return (flags.if_) ? InterruptState::Enabled : InterruptState::Disabled;
}

}