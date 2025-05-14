#include <kernel/time/hpet/timer.h>
#include <kernel/time/hpet/hpet.h>
#include <kernel/time/manager.h>

#include <std/format.h>

namespace kernel {

OwnPtr<HPETTimer> HPETTimer::create(HPET* hpet, u8 irq, u8 id) {
    return OwnPtr<HPETTimer>(new HPETTimer(hpet, irq, id));
}

HPETTimer::HPETTimer(HPET* hpet, u8 irq, u8 id) : IRQHandler(irq), m_hpet(hpet), m_id(id), m_configuration(hpet->timer_configuration_for(id)) {
    TimerConfigurationAndCapabilities configuration(m_configuration->configuration_and_capabilities);

    m_64_bit_capable = configuration.size_capable;
    m_periodic_capable = configuration.periodic_capable;

    configuration.interrupt_route = irq & 0x1F;
    configuration.interrupt_enable = 1;

    m_configuration->configuration_and_capabilities = configuration.value;
    m_configuration->comparator_value = 0;

    this->enable_irq();
}

void HPETTimer::handle_irq() {
    if (!m_is_periodic) {
        this->update();
    }

    TimeManager::tick();
}

void HPETTimer::enable() {
    TimerConfigurationAndCapabilities configuration(m_configuration->configuration_and_capabilities);
    configuration.interrupt_enable = 1;

    m_configuration->configuration_and_capabilities = configuration.value;
    this->enable_irq();
}

void HPETTimer::disable() {
    TimerConfigurationAndCapabilities configuration(m_configuration->configuration_and_capabilities);
    configuration.interrupt_enable = 0;

    m_configuration->configuration_and_capabilities = configuration.value;
    this->disable_irq();
}

void HPETTimer::update() {
    u64 value = m_hpet->frequency() / m_frequency;
    u64 counter = m_hpet->counter() + value;

    m_configuration->comparator_value = counter;
}

bool HPETTimer::set_frequency(size_t frequency) {
    if (frequency > m_hpet->frequency()) {
        return false;
    }

    if (frequency == m_frequency) {
        return true;
    }

    m_frequency = frequency;
    if (!m_is_periodic) {
        this->update();
    }

    return true;    
}

}