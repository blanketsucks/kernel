#include <kernel/process/process.h>
#include <kernel/process/blocker.h>

namespace kernel {

void Process::sys$exit(int status) {
    dbgln("Process exited with status {}.", status);

    m_exit_status = status;
    WaitBlocker::try_wake_all(this, status);

    this->kill();
    __builtin_unreachable();
}


}