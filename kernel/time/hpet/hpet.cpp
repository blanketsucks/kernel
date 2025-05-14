#include <kernel/time/hpet/hpet.h>
#include <kernel/acpi/acpi.h>
#include <kernel/memory/manager.h>

#include <std/format.h>

namespace kernel {

static HPET* s_instance = nullptr;

HPET* HPET::instance() {
    return s_instance;
}

bool HPET::init() {
    if (s_instance) {
        return false;
    }

    auto* hpet = s_instance = new HPET;
    bool result = hpet->initialize();

    if (!result) {
        delete hpet;
        s_instance = nullptr;

        return false;
    }

    return true;
}

bool HPET::initialize() {
    auto* parser = acpi::Parser::instance();
    auto* hpet = parser->find_table<HPETTable>("HPET");

    if (!hpet) {
        return false;
    }

    m_registers = reinterpret_cast<HPETRegisters*>(
        MM->map_physical_region(reinterpret_cast<void*>(hpet->address.address), sizeof(HPETRegisters))
    );

    this->disable();

    m_registers->main_counter_value = 0;
    GeneralCapabilitiesAndID capabilities(m_registers->general_capabilities_and_id);

    m_counter_clock_period = capabilities.counter_clock_period;
    m_minimum_tick = hpet->minimum_tick;

    u64 frequency = 1e15 / m_counter_clock_period;
    u32 mhz = frequency / 1e6;

    dbgln("HPET:");
    dbgln(" - Timer Count: {}", hpet->comparator_count);
    dbgln(" - Frequency: {} Hz ({} MHz)", frequency, mhz);
    dbgln(" - Minimum tick: {}", m_minimum_tick);
    if (hpet->count_size_cap) {
        dbgln(" - Counter size: 64-bit");
    } else {
        dbgln(" - Counter size: 32-bit");
    }

    // Replace the PIT and RTC timers
    if (capabilities.legacy_replacement) {
        m_registers->general_configuration = m_registers->general_configuration | (1 << 1);
    }

    m_timers.append(HPETTimer::create(this, 0, 0));

    dbgln();
    this->enable();

    return true;
}

void HPET::enable() {
    u64 general_configuration = m_registers->general_configuration;
    m_registers->general_configuration = general_configuration | 1;
}

void HPET::disable() {
    u64 general_configuration = m_registers->general_configuration;
    general_configuration &= ~1;

    m_registers->general_configuration = general_configuration;
}

TimerConfiguration* HPET::timer_configuration_for(u8 id) const {
    if (id >= 32) {
        return nullptr;
    }
    
    return &m_registers->timers[id];
}

u64 HPET::counter() {
    return m_registers->main_counter_value;
}

}