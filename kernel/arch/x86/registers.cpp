#include <kernel/arch/x86/registers.h>
#include <kernel/process/threads.h>
#include <std/format.h>

namespace kernel::arch {

extern "C" void _first_yield();

void ThreadRegisters::set_initial_stack_state(Thread* thread) {
    auto& stack = thread->kernel_stack();
    if (!thread->is_kernel()) {
        stack.push<u32>(ss);
        stack.push(esp);
    }

    // Push the registers onto the stack
    stack.push(eflags);
    stack.push(cs);
    stack.push(eip);

    stack.push(eax);
    stack.push(ecx);
    stack.push(edx);
    stack.push(ebx);
    stack.push<u32>(0); // `popad` increments ESP by 4 here so we push a dummy value
    stack.push(ebp);
    stack.push(esi);
    stack.push(edi);

    stack.push(ss); // ds
    stack.push(ss); // es
    stack.push(ss); // fs
    stack.push(ss); // gs

    stack.push(reinterpret_cast<u32>(_first_yield));
    stack.push(ebx); 
    stack.push(esi);
    stack.push(edi);
    stack.push<u32>(0);   // ebp
    stack.push<u32>(0x2); // eflags

    esp0 = stack.value();
}

void dump_registers(const Registers& regs) {
    dbgln("Registers:");
    dbgln("  gs={:#p} fs={:#p} es={:#p} ds={:#p}", regs.gs, regs.fs, regs.es, regs.ds);
    dbgln("  edi={:#p} esi={:#p} ebp={:#p} esp0={:#p}", regs.edi, regs.esi, regs.ebp, regs.esp0);
    dbgln("  ebx={:#p} edx={:#p} ecx={:#p} eax={:#p}", regs.ebx, regs.edx, regs.ecx, regs.eax);
    dbgln("  eip={:#p} cs={:#p} eflags={:#p} esp={:#p} ss={:#p}", regs.eip, regs.cs, regs.eflags, regs.esp, regs.ss);
}

void dump_registers(const InterruptRegisters& regs) {
    dbgln("Interrupt Registers:");
    dbgln("  gs={:#p} fs={:#p} es={:#p} ds={:#p}", regs.gs, regs.fs, regs.es, regs.ds);
    dbgln("  edi={:#p} esi={:#p} ebp={:#p} esp0={:#p}", regs.edi, regs.esi, regs.ebp, regs.esp0);
    dbgln("  ebx={:#p} edx={:#p} ecx={:#p} eax={:#p}", regs.ebx, regs.edx, regs.ecx, regs.eax);
    dbgln("  intno={:#p} errno={:#p}", regs.intno, regs.errno);
    dbgln("  eip={:#p} cs={:#p} eflags={:#p} esp={:#p} ss={:#p}", regs.eip, regs.cs, regs.eflags, regs.esp, regs.ss);
}

}