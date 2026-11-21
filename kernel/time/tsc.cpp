#include <kernel/time/tsc.h>
#include <kernel/time/calibration.h>
#include <kernel/arch/processor.h>

namespace kernel {

TSCClockSource::TSCClockSource() {
    m_frequency = this->calibrate();

    u64 ghz  = m_frequency / 1'000'000'000;
    u64 rest = (m_frequency % 1'000'000'000) / 1'000'000;

    dbgln("TSC frequency calibrated to {}.{} GHz", ghz, rest);
}

u64 TSCClockSource::calibrate() {
    // Check if the CPU exposes core crystal clock information first
    u32 max_basic_leaf = arch::cpuid(0x0).eax;
    if (max_basic_leaf >= 0x15) {
        auto [eax, ebx, ecx, edx] = arch::cpuid(0x15);
        if (ecx != 0 && eax != 0 && ebx != 0) {
            return (ecx * ebx) / eax;
        }
    }

    // Check if hypervisor information is available to us
    if (Processor::current().has_feature(arch::Feature::HYPERVISOR)) {
        u32 max_hypervisor_leaf = arch::cpuid(0x40000000).eax;
        if (max_hypervisor_leaf >= 0x40000010) {
            auto [eax, ebx, ecx, edx] = arch::cpuid(0x40000010);
            if (eax != 0) {
                return eax * 1'000; // TSC frequency (in eax) is reported in KHz
            }
        }
    }

    // Fallback to calibrating using the PIT
    return calibrate_timer(
        []() {
            return arch::rdtsc();
        },
        [](u64 start) {
            return arch::rdtsc() - start;
        }
    );
}

}
