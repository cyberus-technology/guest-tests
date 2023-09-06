/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <cbl/baretest/baretest.hpp>
#include <cpuid.hpp>
#include <lapic_enabler.hpp>
#include <lapic_test_tools.hpp>

using namespace lapic_test_tools;

static bool tsc_adjust_supported()
{
    return cpuid(0x7).ebx & LVL_0000_0007_EBX_TSCADJUST;
}

static bool lapic_supports_tsc_deadline_mode()
{
    return cpuid(0x1).ecx & LVL_0000_0001_ECX_TSCD;
}

TEST_CASE(tsc_only_moves_forward_strictly_monotonic)
{
    static constexpr unsigned REPETITIONS {100000};

    uint64_t tsc1 {rdtsc()};

    for (unsigned i = 0; i < REPETITIONS; ++i) {
        uint64_t tsc2 {rdtsc()};
        BARETEST_ASSERT(tsc2 > tsc1);
        tsc1 = tsc2;
    }
}

static const std::initializer_list<uint64_t> test_tscs {0x13371337ul, 0xf000000000ul, 0xf000000000000000ul};

TEST_CASE(tsc_is_modified_when_writing_to_ia32_time_stamp_counter)
{
    for (auto tsc : test_tscs) {
        wrmsr(x86::msr::IA32_TIME_STAMP_COUNTER, tsc);
        BARETEST_ASSERT(rdtsc() > tsc);
    }
}

TEST_CASE_CONDITIONAL(tsc_is_modified_when_writing_to_ia32_tsc_adjust, tsc_adjust_supported())
{
    for (auto tsc : test_tscs) {
        // Clear all time stamp counter modifications.
        // See SDM 18.17.3: Time Stamp Counter Adjustment
        wrmsr(x86::msr::IA32_TSC_ADJUST, 0);

        wrmsr(x86::msr::IA32_TSC_ADJUST, tsc - rdtsc());

        BARETEST_ASSERT(rdtsc() > tsc);
    }
}

TEST_CASE_CONDITIONAL(local_apic_timer_uses_tsc_as_configured, lapic_supports_tsc_deadline_mode())
{
    lapic_enabler lenabler;

    write_lvt_entry(lvt_entry::TIMER,
                    {.vector = 0x123 /* never triggers because we're executing with interrupts disabled */,
                     .mode = lvt_modes::DEADLINE,
                     .mask = 0});

    for (auto tsc : test_tscs) {
        // Set the TSC to a specific value
        wrmsr(x86::msr::IA32_TIME_STAMP_COUNTER, tsc);

        // Program a TSC value that is in the past
        wrmsr(x86::msr::IA32_TSC_DEADLINE, tsc - 1);

        // Lapic should immediately mark the IRQ as fired by clear the MSR. Immediately here
        // means as soon as the Lapic state machine as processed the timer write. This can
        // take a few cycles depending on the underlying hardware. Make sure we try this at least
        // a couple of times.
        unsigned retry_counter {100};
        uint64_t read_back {~0ul};

        while (--retry_counter) {
            read_back = rdmsr(x86::msr::IA32_TSC_DEADLINE);
            if (read_back == 0) {
                break;
            }
        }

        BARETEST_ASSERT(read_back == 0);
    }
}

BARETEST_RUN;
