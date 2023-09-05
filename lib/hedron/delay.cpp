/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <hedron/delay.hpp>
#include <trace.hpp>
#include <x86asm.hpp>

static uint64_t delay_tsc_freq_khz {0};

void initialize_delay_subsystem(uint32_t tsc_freq_khz_)
{
    delay_tsc_freq_khz = tsc_freq_khz_;
}

void udelay(uint64_t us)
{
    PANIC_UNLESS(delay_tsc_freq_khz, "TSC frequency not initialized!");

    auto tsc_start {rdtsc()};
    uint64_t ticks = us * delay_tsc_freq_khz / 1000;
    auto tsc_end {tsc_start + ticks};
    while (rdtsc() < tsc_end) {
        cpu_pause();
    }
}
