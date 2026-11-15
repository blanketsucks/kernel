#include <kernel/process/scheduler.h>
#include <kernel/process/process.h>
#include <kernel/process/threads.h>
#include <kernel/posix/sys/wait.h>

namespace kernel {

ErrorOr<FlatPtr> Process::sys$waitpid(pid_t pid, int* status, int options) {
    this->validate_write(status, sizeof(int));

    ASSERT(pid > 0); // No waiting for any child for now.

    auto* process = Scheduler::get_process(pid);
    if (options & WNOHANG) {
        if (!process) {
            return Error(ECHILD);
        } else if (process->parent_id() != this->id()) {
            return Error(ECHILD);
        }

        if (process->state() == Zombie) {
            *status = __WIFEXITED | process->exit_status();
            process->set_state(Dead);

            return pid;
        }

        return 0;
    }

    process->m_exit_wq.prepare_and_wait([]() {});
    *status = __WIFEXITED | process->exit_status();

    return pid;
}


}
