#include <kernel/time/apic.h>
#include <kernel/time/pit.h>
#include <kernel/arch/interrupts/apic.h>

#include <std/format.h>
#include <std/asserts.h>

namespace kernel {

RefPtr<APICTimer> APICTimer::create(u8 irq) {
    auto timer = RefPtr(new APICTimer(irq));

    timer->register_interrupt_handler();
    timer->calibrate();

    return timer;
}

APICTimer::APICTimer(u8 irq) : InterruptHandler(irq) {}

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

    this->disable_local_timer();
}


void APICTimer::start_periodic(u64 hz) {
    this->register_interrupt_handler();
    this->enable_local_timer(hz);
}

void APICTimer::start_oneshot(u64) {
    ASSERT_MSG(false, "APICTimer::start_oneshot is not implemented yet.");
}

void APICTimer::stop() {
    this->disable_local_timer();
    this->unregister_interrupt_handler();
}

void APICTimer::enable_local_timer(u64 frequency) {
    APIC::instance().configure_local_timer(m_frequency / frequency, true);
}

void APICTimer::disable_local_timer() {
    APIC::instance().configure_local_timer(0, false);
}

}
