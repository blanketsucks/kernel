#include <kernel/arch/tss.h>
#include <kernel/process/threads.h>

namespace kernel::arch {

void TSS::init(Thread* initial_thread) {
    auto& regs = initial_thread->registers();
    iomap_base = sizeof(TSS);

    rsp0l = regs.sp0() & 0xFFFFFFFF;
    rsp0h = regs.sp0() >> 32;
}

}
