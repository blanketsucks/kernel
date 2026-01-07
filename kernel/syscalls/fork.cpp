#include <kernel/process/process.h>
#include <kernel/process/scheduler.h>

namespace kernel {

Process* Process::fork(arch::Registers* registers) {
    auto* process = new Process(name(), this);
    auto* thread = new Thread(process, registers);

    process->add_thread(thread);
    return process;
}

ErrorOr<FlatPtr> Process::sys$fork(arch::Registers* registers) {
    auto* process = this->fork(registers);
    Scheduler::add_process(process);

    return process->id();
}

}