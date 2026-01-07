#include <kernel/process/process.h>
#include <kernel/process/threads.h>

namespace kernel {

ErrorOr<FlatPtr> Process::sys$getpid() {
    return m_id;
}

ErrorOr<FlatPtr> Process::sys$getppid() {
    return m_parent_id;
}

ErrorOr<FlatPtr> Process::sys$gettid() {
    return Thread::current()->id();
}

}