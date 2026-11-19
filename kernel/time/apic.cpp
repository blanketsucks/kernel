#include <kernel/time/apic.h>
#include <kernel/time/pit.h>
#include <kernel/time/calibration.h>
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
    auto& apic = APIC::instance();

    m_frequency = calibrate_timer(
        [&]() -> u64 {
            apic.configure_local_timer(0xffffffff);
            return 0xffffffff;
        },
        [&](u64 start) -> u64 {
            u64 current = apic.read_apic_timer_ticks();
            apic.configure_local_timer(0, false);

            return start - current;
        }
    );

    // calibrate_timer would have called configure_local_timer(0, false) at this point so
    // no need to do it ourselves.

    u64 khz  = m_frequency / 1'000'000;
    u64 rest = (m_frequency % 1'000'000) / 1'000;

    dbgln("LAPIC timer calibrated to {}.{} MHz", khz, rest);
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
