#pragma once

#include <kernel/time/pit.h>
#include <kernel/arch/interrupts.h>

#include <std/format.h>

namespace kernel {

static constexpr bool DEBUG_CALIBRATION = false;

template<size_t NUM_SAMPLES = 5, size_t SAMPLE_MS = 20, typename Begin, typename End>
u64 calibrate_timer(Begin&& begin, End&& end) {
    static_assert(NUM_SAMPLES > 0 && (NUM_SAMPLES % 2) == 1, "NUM_SAMPLES needs to be an odd number");
    static_assert(SAMPLE_MS > 0);

    u64 samples[NUM_SAMPLES];
    for (size_t i = 0; i < NUM_SAMPLES; ++i) {
        u64 ticks = 0;

        {
            arch::InterruptDisabler disabler; // In case interrupts are enabled when at this point.

            u64 start = begin();
            PIT::wait_ms(SAMPLE_MS);
            ticks = end(start);
        }

        u64 hz = (ticks * 1000ull + (SAMPLE_MS / 2)) / SAMPLE_MS;
        if constexpr (DEBUG_CALIBRATION) {
            dbgln("Run {}: Calculated frequency is {}Hz", i, hz);
        }

        samples[i] = hz;
    }

    for (size_t i = 1; i < NUM_SAMPLES; ++i) {
        u64 key = samples[i];
        size_t j = i;

        while (j > 0 && samples[j - 1] > key) {
            samples[j] = samples[j - 1];
            --j;
        }

        samples[j] = key;
    }

    return samples[NUM_SAMPLES / 2];
}

}
