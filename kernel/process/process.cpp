#include <kernel/process/process.h>
#include <kernel/process/threads.h>
#include <kernel/process/scheduler.h>

namespace kernel {

Process* Process::create_kernel_process(String name, void (*entry)()) {
    return new Process(1, move(name), true, entry);
}

Process::Process(pid_t id, String name, bool kernel, void (*entry)()) : m_id(id), m_name(move(name)), m_kernel(kernel) {
    if (!kernel) {
        m_page_directory = memory::PageDirectory::create_user();
    } else {
        m_page_directory = memory::PageDirectory::kernel_page_directory();
    }

    auto thread = Thread::create(m_id, "main", this, entry);
    this->add_thread(thread);
}

void Process::add_thread(Thread* thread) {
    m_threads.set(thread->id(), thread);
}

void Process::remove_thread(Thread* thread) {
    m_exit_values.set(thread->id(), thread->m_exit_value);
    m_threads.remove(thread->id());
}

Thread* Process::get_thread(u32 id) const {
    auto iterator = m_threads.find(id);
    if (iterator != m_threads.end()) {
        return iterator->value;
    }

    return nullptr;
}

Thread* Process::get_main_thread() const {
    return this->get_thread(m_id);
}

Thread* Process::spawn(String name, void (*entry)()) {
    auto thread = Thread::create(generate_id(), move(name), this, entry);
    this->add_thread(thread);

    Scheduler::queue(thread);
    return thread;
}

void Process::notify_exit(Thread* thread) {
    this->remove_thread(thread);
}

}