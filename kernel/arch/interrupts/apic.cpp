#include <kernel/arch/cpu.h>
#include <kernel/arch/interrupts/apic.h>
#include <kernel/arch/processor.h>
#include <kernel/memory/manager.h>
#include <kernel/process/scheduler.h>
#include <kernel/acpi/acpi.h>
#include <kernel/arch/idt.h>
#include <kernel/arch/io.h>

namespace kernel {

#define LVT_DISABLE (1 << 16)

extern "C" void ap_boot_start();
extern "C" void ap_boot_end();

extern "C" void init_ap(FlatPtr);

extern "C" FlatPtr ap_init_cr3;
extern "C" FlatPtr ap_init_stacks;
extern "C" FlatPtr ap_entry_function;

static PhysicalAddress get_apic_base() {
    u32 low, high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(APIC::IA32_APIC_BASE));

    if constexpr (sizeof(PhysicalAddress) > 4) {
        return PhysicalAddress { (low & 0xfffff000) | (static_cast<u64>(high & 0x0f) << 32) };
    } else {
        return PhysicalAddress { low & 0xfffff000 };
    }
}

static void set_apic_base(PhysicalAddress base) {
    u32 low = (base & 0xfffff0000) | APIC::IA32_APIC_BASE_ENABLE;
    u32 high = 0;

    if constexpr (sizeof(PhysicalAddress) > 4) {
        high = (base >> 32) & 0x0f;
    }

    asm volatile("wrmsr" :: "a"(low), "d"(high), "c"(APIC::IA32_APIC_BASE));
}

static APIC s_apic {};
static bool s_initialized = false;

APIC& APIC::instance() {
    return s_apic;
}

bool APIC::is_initialized() {
    return s_initialized;
}

u32 APIC::processors() {
    return s_apic.m_processors;
}

void APIC::initialize() {
    s_initialized = true;
    s_apic.init();
}

void APIC::init() {
    PhysicalAddress base = get_apic_base();
    set_apic_base(base);

    m_apic_base = VirtualAddress { MUST(MM->map_physical_region(base, PAGE_SIZE)) };

    auto* madt = ACPIParser::find<acpi::MADT>();
    ASSERT(madt);

    madt->for_each_entry([this](acpi::MADTHeader* entry) {
        if (entry->type != acpi::MADTEntryType::ProcessorLocalAPIC) {
            return;
        }

        auto* lapic = reinterpret_cast<acpi::ProcessorLocalAPIC*>(entry);
        if (!(lapic->flags & 0x1)) {
            return;
        }

        m_processors++;
    });

    dbgln("APIC Processors found: {}", m_processors);
    this->enable(0);
}

void APIC::enable(int cpu) {
    u32 ld = read_reg(LogicalDestination) & 0x00ffffff;

    write_reg(LogicalDestination, ld | (cpu << 24));
    write_reg(DestinationFormat, 0xff000000);

    u32 siv = read_reg(SpuriousInterruptVector) | (SPURIOUS_INTERRUPT_VECTOR + IRQ_VECTOR_BASE);
    write_reg(SpuriousInterruptVector, siv | 0x100);

    write_reg(LVTTimer, LVT_DISABLE);
    write_reg(LVTLint0, LVT_DISABLE);
    write_reg(LVTLint1, LVT_DISABLE);

    write_reg(TaskPriority, 0);
}

RefPtr<APICTimer> APIC::create_timer() {
    if (m_timer) {
        return m_timer;
    }

    m_timer = APICTimer::create(TIMER_INTERRUPT_VECTOR);
    return m_timer;
}

void APIC::configure_local_timer(u32 initial_count) {
    write_reg(TimerInitialCount, 0);
    write_reg(LVTTimer, (TIMER_INTERRUPT_VECTOR + IRQ_VECTOR_BASE) | 0x20000);

    write_reg(TimerDivideConfiguration, 0x3);
    write_reg(TimerInitialCount, initial_count);
}

u32 APIC::read_apic_timer_ticks() {
    return read_reg(TimerCurrentCount);
}

Processor* APIC::get_ap_processor(int ap) {
    return s_apic.m_ap_processors[ap];
}

Thread* APIC::get_ap_idle_thread(int ap) {
    return s_apic.m_ap_idle_threads[ap];
}

void APIC::setup_ap_trampoline() {
    u32 to_boot = m_processors - 1;
    if (!to_boot) {
        return;
    }

    // TODO: Move this to somewhere else as it doesn't really have to do anything with setting up the
    //       trampoline itself.
    m_ap_processors.resize(to_boot);
    for (size_t i = 0; i < to_boot; i++) {
        m_ap_processors[i] = new Processor;
    }

    m_ap_idle_threads.resize(to_boot);
    for (size_t i = 0; i < to_boot; i++) {
        m_ap_idle_threads[i] = Scheduler::create_idle_thread(i + 1);
    }

    FlatPtr ap_start_address = reinterpret_cast<FlatPtr>(&ap_boot_start);
    size_t size = (reinterpret_cast<FlatPtr>(&ap_boot_end) - ap_start_address) + (sizeof(FlatPtr) * to_boot);

    m_trampoline = PhysicalAddress { 0x8000 };
    MUST(MM->identity_map_region(m_trampoline, size));

    memcpy(m_trampoline.to_ptr(), reinterpret_cast<void*>(&ap_boot_start), size);

    auto compute_ap_variable_address = [&](FlatPtr& var) {
        return m_trampoline.to_ptr() + (reinterpret_cast<FlatPtr>(&var) - ap_start_address);
    };

    auto write_ap_variable = [&](FlatPtr& var, FlatPtr value) {
        u8* address = compute_ap_variable_address(var);
        *reinterpret_cast<FlatPtr*>(address) = value;
    };

    m_ap_stacks.reserve(to_boot);
    for (size_t i = 0; i < to_boot; i++) {
        auto* stack = MUST(MM->allocate_kernel_region(32 * KiB));
        m_ap_stacks.append(VirtualAddress { stack });
    }

    auto* ap_init_stack_array = reinterpret_cast<FlatPtr*>(compute_ap_variable_address(ap_init_stacks));
    for (size_t i = 0; i < to_boot; i++) {
        ap_init_stack_array[i] = m_ap_stacks[i].offset(32 * KiB);
    }

    auto* page_directory = arch::PageDirectory::kernel_page_directory();

    write_ap_variable(ap_init_cr3, page_directory->cr3());
    write_ap_variable(ap_entry_function, FlatPtr(&init_ap));
}

void APIC::do_boot_all() {
    u32 to_boot = m_processors - 1;
    if (!to_boot) {
        return;
    }

    this->setup_ap_trampoline();

    write_icr(0, 0, ICR::INIT, ICR::Physical, ICR::Assert, ICR::Edge, ICR::AllButSelf);
    io::wait(10 * 1000);

    for (int i = 0; i < 2; i++) {
        write_icr(0x08, 0, ICR::SIPI, ICR::Physical, ICR::Assert, ICR::Edge, ICR::AllButSelf);
        io::wait(200);
    }
}

void APIC::boot_all() {
    s_apic.do_boot_all();
}

void APIC::eoi() {
    write_reg(EOI, 0);
}

void APIC::write_reg(u32 reg, u32 value) {
    *reinterpret_cast<volatile u32*>(m_apic_base.to_ptr() + reg) = value;
}

u32 APIC::read_reg(u32 reg) {
    return *reinterpret_cast<volatile u32*>(m_apic_base.to_ptr() + reg);
}

void APIC::write_icr(
    u8 vector,
    u8 destination,
    u8 delivery_mode,
    u8 destination_mode,
    u8 level,
    u8 trigger_mode,
    u8 destination_type
) {
    ICR cmd = {{
        vector,
        delivery_mode,
        destination_mode,
        0,
        level,
        trigger_mode,
        destination_type,
        destination
    }};

    write_reg(InterruptCommandRegister, cmd.low);
    write_reg(InterruptCommandRegister + 0x10, cmd.high);
}

}
