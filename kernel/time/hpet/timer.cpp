#include <kernel/time/hpet/timer.h>
#include <kernel/time/hpet/hpet.h>
#include <kernel/time/manager.h>
#include <kernel/arch/processor.h>

#include <std/format.h>
#include <std/asserts.h>

namespace kernel {

RefPtr<HPETTimer> HPETTimer::create(HPET* hpet, u8 irq, u8 id) {
    return RefPtr<HPETTimer>(new HPETTimer(hpet, irq, id));
}

HPETTimer::HPETTimer(
    HPET* hpet, u8 irq, u8 id
) : IRQHandler(irq), m_hpet(hpet), m_id(id), m_configuration(hpet->timer_configuration_for(id)) {
    TimerConfigurationAndCapabilities configuration(m_configuration->configuration_and_capabilities);

    m_periodic_capable = configuration.periodic_capable;

    configuration.interrupt_route = irq & 0x1F;

    m_configuration->configuration_and_capabilities = configuration.value;
    m_configuration->comparator_value = 0;
}

void HPETTimer::handle_irq() {
    dispatch();
    this->update();
}

void HPETTimer::start_periodic(u64 hz) {
    m_target_frequency = hz;

    this->enable();
    this->update();
}

void HPETTimer::start_oneshot(u64) {
    ASSERT_MSG(false, "HPETTimer::start_oneshot is not implemented yet.");
}

void HPETTimer::enable() {
    TimerConfigurationAndCapabilities configuration(m_configuration->configuration_and_capabilities);
    configuration.interrupt_enable = 1;

    m_configuration->configuration_and_capabilities = configuration.value;
    m_configuration->comparator_value = 0;

    this->enable_irq();
}

void HPETTimer::stop() {
    TimerConfigurationAndCapabilities configuration(m_configuration->configuration_and_capabilities);
    configuration.interrupt_enable = 0;

    m_configuration->configuration_and_capabilities = configuration.value;
    m_configuration->comparator_value = 0;

    this->disable_irq();
}

u64 HPETTimer::frequency() const {
    return 1'000'000'000'000'000ull / m_hpet->counter_clock_period();
}

void HPETTimer::update() {
    u64 value = frequency() / m_target_frequency;
    u64 counter = m_hpet->counter() + value;

    m_configuration->comparator_value = counter;
}

}
