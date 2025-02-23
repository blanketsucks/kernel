#include <kernel/arch/processor.h>
#include <kernel/arch/registers.h>
#include <kernel/process/threads.h>

namespace kernel {

extern "C" void _switch_context(arch::ThreadRegisters*, arch::ThreadRegisters*);
extern "C" void _switch_context_no_state(arch::ThreadRegisters*);

static Processor s_instance;

Processor& Processor::instance() {
    return s_instance;
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

}