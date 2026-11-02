#include <kernel/arch/interrupts/ioapic.h>
#include <kernel/arch/interrupts/apic.h>
#include <kernel/arch/interrupts/manager.h>
#include <kernel/arch/cpu.h>
#include <kernel/memory/manager.h>

namespace kernel {

IOAPIC::IOAPIC(PhysicalAddress address, u16 gsi_base) : m_gsi_base(gsi_base) {
    m_base = VirtualAddress { MUST(MM->map_physical_region(address, PAGE_SIZE)) };

    m_id = (read_reg(ID) >> 24) & 0xFF;
    m_version = (read_reg(Version) & 0xFF);
    m_max_redirection_entry = (read_reg(Version) >> 16) + 1;
    
    dbgln("IOAPIC (ID={}):", m_id);
    dbgln("  - GSI Base: {}", m_gsi_base);
    dbgln("  - Max Redirection Entry: {}", m_max_redirection_entry);
    dbgln("  - Version: {}", m_version);
    dbgln();

    for (size_t i = 0; i < m_max_redirection_entry; i++) {
        this->set_redirection_entry_mask(i, true);
    }
}

void IOAPIC::enable(u16 irq) {
    auto opt = this->find_redirection_entry(irq);
    if (opt.has_value()) {
        this->set_redirection_entry_mask(opt.value(), false);
        return;
    }

    u8 index = this->configure_interrupt_redirection_entry(irq);
    this->set_redirection_entry_mask(index, false);   
}

void IOAPIC::disable(u16 irq) {
    auto opt = this->find_redirection_entry(irq);
    if (opt.has_value()) {
        this->set_redirection_entry_mask(opt.value(), true);
        return;
    }

    u8 index = this->configure_interrupt_redirection_entry(irq);
    this->set_redirection_entry_mask(index, true);   
}

void IOAPIC::eoi(u16) {
    APIC::instance().eoi();
}

Optional<u8> IOAPIC::find_redirection_entry(u16 irq) {
    for (size_t i = 0; i < m_max_redirection_entry; i++) {
        if (get_redirection_entry_vector(i) == (irq + IRQ_VECTOR_BASE)) {
            return i;
        }
    }

    return {};
}

u8 IOAPIC::configure_interrupt_redirection_entry(u16 irq) {
    for (auto& override : InterruptManager::instance()->interrupt_source_overrides()) {
        if (override.irq != irq) {
            continue;
        }

        bool polarity     = override.polarity == 3;
        bool trigger_mode = override.trigger_mode == 3;

        u8 index = override.gsi - m_gsi_base;
        auto entry = read_redirection_entry(index);

        entry.vector = irq + IRQ_VECTOR_BASE;
        entry.delivery_mode = 0; // FIXED
        entry.destination_mode = 0; // Physical Destination
        entry.pin_polarity = polarity;
        entry.trigger_mode = trigger_mode;
        entry.mask = 1;
        entry.destination = 0;

        write_redirection_entry(index, entry);
        return index;
    }

    u8 index = irq - m_gsi_base;
    auto entry = read_redirection_entry(index);

    entry.vector = irq + IRQ_VECTOR_BASE;
    entry.delivery_mode = 0; // FIXED
    entry.destination_mode = 0; // Physical Destination
    entry.pin_polarity = 0;
    entry.trigger_mode = 0;
    entry.mask = 1;
    entry.destination = 0;

    write_redirection_entry(index, entry);
    return index;
}

RedirectionEntry IOAPIC::read_redirection_entry(u8 index) {
    u32 low  = read_reg(RedirectionEntryBase + (index * 2));
    u32 high = read_reg(RedirectionEntryBase + (index * 2) + 1);

    return { .low = low, .high = high };
}

void IOAPIC::write_redirection_entry(u8 index, RedirectionEntry entry) {
    write_reg(RedirectionEntryBase + (index * 2), entry.low);
    write_reg(RedirectionEntryBase + (index * 2) + 1, entry.high);
}

void IOAPIC::set_redirection_entry_mask(u8 index, bool value) {
    auto entry = read_redirection_entry(index);
    if (entry.mask == value) {
        return;
    }

    entry.mask = value;
    write_redirection_entry(index, entry);
}

u8 IOAPIC::get_redirection_entry_vector(u8 index) {
    return read_redirection_entry(index).vector;
}

u32 IOAPIC::read_reg(u32 reg) {
    *reinterpret_cast<u32*>(m_base.to_ptr()) = reg;
    return *reinterpret_cast<u32*>(m_base.to_ptr() + 0x10);
}

void IOAPIC::write_reg(u32 reg, u32 value) {
    *reinterpret_cast<u32*>(m_base.to_ptr())        = reg;
    *reinterpret_cast<u32*>(m_base.to_ptr() + 0x10) = value;
}

}