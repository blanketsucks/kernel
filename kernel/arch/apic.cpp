#include <kernel/arch/apic.h>
#include <kernel/acpi/acpi.h>
#include <kernel/arch/pic.h>
#include <kernel/memory/manager.h>
#include <kernel/serial.h>

namespace kernel::apic {

static VirtualAddress g_apic_base;

PhysicalAddress get_apic_base() {
    u32 low, high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(IA32_APIC_BASE));

    if constexpr (sizeof(PhysicalAddress) > 4) {
        return (low & 0xfffff000) | (static_cast<u64>(high & 0x0f) << 32);
    } else {
        return low & 0xfffff000;
    }
}

void set_apic_base(PhysicalAddress base) {
    u32 low = (base & 0xfffff0000) | IA32_APIC_BASE_ENABLE;
    u32 high = 0;

    if constexpr (sizeof(PhysicalAddress) > 4) {
        high = (base >> 32) & 0x0f;
    }

    asm volatile("wrmsr" :: "a"(low), "d"(high), "c"(IA32_APIC_BASE));
}

void write_reg(APICRegisters reg, u32 value) {
    write_reg(to_underlying(reg), value);
}

void write_reg(u32 reg, u32 value) {
    *reinterpret_cast<volatile u32*>(g_apic_base.to_ptr() + reg) = value;
}

u32 read_reg(APICRegisters reg) {
    return read_reg(to_underlying(reg));
}

u32 read_reg(u32 reg) {
    return *reinterpret_cast<volatile u32*>(g_apic_base.to_ptr() + reg);
}

void init() {
    pic::disable();

    PhysicalAddress base = get_apic_base();
    set_apic_base(base);

    // Map the APIC registers
    g_apic_base = VirtualAddress { MM->map_physical_region(reinterpret_cast<void*>(base), PAGE_SIZE) };

    u32 value = read_reg(APICRegisters::SpuriousInterruptVector);
    write_reg(APICRegisters::SpuriousInterruptVector, value | SPURIOUS_INTERRUPT_VECTOR | 0x100);

    dbgln("APIC initialized at physical address {:#x}, mapped at virtual address {:#x}", base, g_apic_base);

    auto* madt = ACPIParser::find<acpi::MADT>();
    if (!madt) {
        dbgln("No MADT found!");
        return;
    }

    size_t length = madt->header.length - sizeof(acpi::SDTHeader);
    u8* entries = reinterpret_cast<u8*>(madt) + sizeof(acpi::MADT);

    dbgln("length: {}", length);

    size_t offset = 0;
    while (offset < length) {
        auto* header = reinterpret_cast<acpi::MADTHeader*>(entries + offset);

        switch (header->type) {
            case acpi::MADTEntryType::IOAPIC: {
                auto* ioapic = reinterpret_cast<acpi::IOAPIC*>(header);

                dbgln("IOAPIC found: ID={}, Address={:#x}, Global IRQ Base={}", ioapic->io_apic_id, ioapic->io_apic_address, ioapic->global_system_interrupt_base);
            }
            case acpi::MADTEntryType::InterruptSourceOverride: {
                auto* iso = reinterpret_cast<acpi::InterruptSourceOverride*>(header);

                dbgln("Interrupt Source Override: Bus Source={}, IRQ Source={}, Global IRQ={}, Flags={:#x}", iso->bus_source, iso->irq_source, iso->global_system_interrupt, iso->flags);
            } break;
            default: break;
        }


        offset += header->length;
    }
}


}