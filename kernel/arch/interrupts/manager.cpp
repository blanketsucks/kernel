#include <kernel/arch/interrupts/manager.h>
#include <kernel/arch/interrupts/ioapic.h>
#include <kernel/arch/interrupts/pic.h>
#include <kernel/acpi/acpi.h>
#include <kernel/panic.h>

#include <std/format.h>

namespace kernel {

static InterruptManager* s_instance = nullptr;

InterruptManager* InterruptManager::instance() {
    return s_instance;
}

void InterruptManager::initialize() {
    s_instance = new InterruptManager;
    s_instance->init();
}

void InterruptManager::init() {
    m_controllers.append(PIC::create());
    enumerate_ioapics();
}

RefPtr<InterruptController> InterruptManager::controller_for(u16 irq) {
    return s_instance->find_responsible_controller(irq);
}

RefPtr<InterruptController> InterruptManager::find_responsible_controller(u16 irq) {
    for (auto& controller : m_controllers) {
        u16 base = controller->gsi_base();
        u16 end  = controller->gsi_base() + controller->vector_count();

        if (irq >= base && irq < end) {
            return controller;
        }
    }

    ASSERT(false, "No interrupt controller found for IRQ.");
}

void InterruptManager::enumerate_ioapics() {
    auto* madt = ACPIParser::find<acpi::MADT>();
    if (!madt) {
        dbgln("No MADT found!");
        return;
    }

    madt->for_each_entry([this](acpi::MADTHeader* header) {
        switch (header->type) {
            case acpi::MADTEntryType::IOAPIC: {
                auto* ioapic = reinterpret_cast<acpi::IOAPIC*>(header);

                m_controllers.append(
                    IOAPIC::create(
                        PhysicalAddress { ioapic->io_apic_address },
                        ioapic->global_system_interrupt_base
                    )
                );

                dbgln("IOAPIC found: ID={}, Address={:#x}, Global IRQ Base={}", ioapic->io_apic_id, ioapic->io_apic_address, ioapic->global_system_interrupt_base);
            } break;
            case acpi::MADTEntryType::InterruptSourceOverride: {
                auto* iso = reinterpret_cast<acpi::InterruptSourceOverride*>(header);

                m_iso.append({ 
                    .bus   = iso->bus_source,
                    .irq   = iso->irq_source,
                    .gsi   = iso->global_system_interrupt,
                    .flags = iso->flags
                });

                dbgln("Overriding INT #{} with GSI {} for bus {}", iso->irq_source, iso->global_system_interrupt, iso->bus_source);
            } break;
            default: break;
        }
    });
}

}