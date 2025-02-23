#pragma once

#include <kernel/arch/tss.h>
#include <kernel/arch/cpu.h>

namespace kernel {

class Processor {
public:
    static void init();

    // For now there is only one instance but where we have SMP we will have multiple instances
    static Processor& instance();

    arch::TSS& tss() { return m_tss; }

    void initialize_context_switching(Thread* initial_thread);
    void switch_context(Thread* old, Thread* next);

    bool has_feature(arch::CPUFeatures feature) const { return std::has_flag(m_features, feature); }

private:
    void* m_user_stack;
    arch::TSS m_tss;

    arch::CPUFeatures m_features;
};

}