#include <kernel/arch/interrupts/apic.h>
#include <kernel/acpi/acpi.h>
#include <kernel/memory/manager.h>

namespace kernel {

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

APIC& APIC::instance() {
    return s_apic;
}

void APIC::initialize() {
    s_apic.init();
}

void APIC::init() {
    PhysicalAddress base = get_apic_base();
    set_apic_base(base);

    m_apic_base = VirtualAddress { MUST(MM->map_physical_region(base, PAGE_SIZE)) };

    write_reg(SpuriousInterruptVector, read_reg(SpuriousInterruptVector) | SPURIOUS_INTERRUPT_VECTOR | 0x100);

    auto* madt = ACPIParser::find<acpi::MADT>();
    ASSERT(madt, "No MADT found.");

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

}