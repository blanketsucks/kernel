#include <kernel/arch/x86/registers.h>
#include <std/format.h>

namespace kernel::arch {

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