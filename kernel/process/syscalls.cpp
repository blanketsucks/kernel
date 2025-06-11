#include <kernel/process/process.h>
#include <kernel/process/syscalls.h>
#include <kernel/process/threads.h>

namespace kernel {

using SyscallHandler = ErrorOr<FlatPtr>(Process::*)(FlatPtr, FlatPtr, FlatPtr, FlatPtr);

static const SyscallHandler SYSCALL_HANDLERS[] = {
#define Op(name) reinterpret_cast<SyscallHandler>(&Process::sys$##name),
    __SYSCALL_LIST(Op)
#undef Op
};

static constexpr size_t SYSCALL_COUNT = sizeof(SYSCALL_HANDLERS) / sizeof(SyscallHandler);

FlatPtr Process::handle_syscall(arch::Registers* registers) {
    FlatPtr syscall, arg1, arg2, arg3, arg4;

    registers->capture_syscall_arguments(syscall, arg1, arg2, arg3, arg4);
    ErrorOr<FlatPtr> result = { FlatPtr(nullptr) };

    if (syscall == SYS_fork) {
        result = this->sys$fork(*registers);
    } else if (syscall == SYS_exit) {
        this->sys$exit(arg1); // sys$exit never returns so it needs special handling
    } else {
        if (syscall >= SYSCALL_COUNT || syscall < 0) {
            return -ENOSYS;
        }

        auto handler = SYSCALL_HANDLERS[syscall];
        if (!handler) {
            return -ENOSYS;
        }

        result = (this->*handler)(arg1, arg2, arg3, arg4);
    }

    if (result.is_err()) {
        return -result.error().code();
    } else {
        return result.value();
    }
}

}