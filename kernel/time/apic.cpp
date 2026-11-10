#include <kernel/time/apic.h>
#include <kernel/time/pit.h>
#include <kernel/arch/interrupts/apic.h>

#include <std/format.h>

namespace kernel {

RefPtr<APICTimer> APICTimer::create(u8 irq) {
    auto timer = RefPtr(new APICTimer(irq));

    timer->register_interrupt_handler();
    timer->calibrate();

    return timer;
}

APICTimer::APICTimer(u8 irq) : Timer(irq) {}

void APICTimer::eoi() {
    APIC::instance().eoi();
}

void APICTimer::calibrate() {
    // TODO: Calibrate using a more accurate method
    auto& apic = APIC::instance();

    apic.configure_local_timer(0xffffffff);
    PIT::wait_ms(10);

    u32 elapsed = 0xffffffff - apic.read_apic_timer_ticks();
    m_frequency = elapsed * 100;

    this->enable_local_timer();
}

void APICTimer::enable_local_timer(u32 frequency) {
    APIC::instance().configure_local_timer(m_frequency / frequency);
}

}
