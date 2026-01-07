#include <kernel/process/scheduler.h>
#include <kernel/process/process.h>
#include <kernel/process/threads.h>
#include <kernel/process/blocker.h>
#include <kernel/posix/sys/wait.h>

namespace kernel {

ErrorOr<FlatPtr> Process::sys$waitpid(pid_t pid, int* status, int options) {
    this->validate_write(status, sizeof(int));

    if (options & WNOHANG) {
        auto* process = Scheduler::get_process(pid);
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

    auto* thread = Thread::current();
    auto* blocker = WaitBlocker::create(thread, pid);

    thread->block(blocker);
    *status = blocker->status();

    return pid;
}


}