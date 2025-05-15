#include <kernel/process/process.h>
#include <kernel/process/syscalls.h>
#include <kernel/process/threads.h>

namespace kernel {

using SyscallHandler = ErrorOr<FlatPtr>(Process::*)(FlatPtr, FlatPtr, FlatPtr, FlatPtr);

FlatPtr Process::handle_syscall(arch::Registers* registers) {
    FlatPtr syscall, arg1, arg2, arg3, arg4;

    registers->capture_syscall_arguments(syscall, arg1, arg2, arg3, arg4);
    ErrorOr<FlatPtr> result = { FlatPtr(nullptr) };

    if (syscall == SYS_FORK) {
        result = this->sys$fork(*registers);
    } else {
        switch (syscall) {
    #define Op(name, func)                                                             \
            case name: {                                                               \
                auto handler = reinterpret_cast<SyscallHandler>(&Process::sys$##func); \
                result = (this->*handler)(arg1, arg2, arg3, arg4); break;              \
            }
            __SYSCALL_LIST(Op)
    #undef Op
            default:
                return -ENOSYS;
        }
    }

    if (result.is_err()) {
        return -result.error().err();
    } else {
        return result.value();
    }
}

}