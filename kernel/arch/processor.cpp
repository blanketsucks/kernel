#include <kernel/arch/processor.h>
#include <kernel/arch/registers.h>
#include <kernel/process/threads.h>
#include <kernel/arch/cpu.h>

namespace kernel {

extern "C" void _switch_context(arch::ThreadRegisters*, arch::ThreadRegisters*);
extern "C" void _switch_context_no_state(arch::ThreadRegisters*);

static Processor s_instance;

Processor& Processor::instance() {
    return s_instance;
}

void Processor::preinit() {
    m_features = arch::cpu_features();
    u32 eax, ebx, ecx, edx;
    arch::cpuid(0x80000000, eax, ebx, ecx, edx);

    u32 max_extended_leaf = eax;
    if (max_extended_leaf >= 0x80000001) {
        arch::cpuid(0x80000001, eax, ebx, ecx, edx);
        m_has_nx = (edx & (1 << 20)) != 0;
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

    dbgln("Processor:");
    dbgln(" - Brand: {}", m_brand);
    dbgln(" - Max Physical Address Width: {} bits", m_max_physical_address_width);
    dbgln(" - Max Virtual Address Width: {} bits", m_max_virtual_address_width);
    dbgln(" - Features: {}", features_string());

    dbgln();
}

void Processor::initialize_context_switching(Thread* initial_thread) {
    m_tss.init(initial_thread);
    _switch_context_no_state(&initial_thread->registers());
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