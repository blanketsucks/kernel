#include <kernel/process/process.h>

namespace kernel {

void Process::sys$exit(int status) {
    dbgln("Process exited with status {}.", status);

    m_exit_status = status;
    m_exit_wq.wake_all();

    this->kill();
    __builtin_unreachable();
}


}
