/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <cstddef>
#include <toyos/baretest/baretest.hpp>
#include <toyos/x86/x86asm.hpp>

TEST_CASE(tsc_is_monotonous)
{
    // Run for at least some minutes.
    static constexpr size_t REPETITIONS{ 200 };

    for (size_t round = 0; round < REPETITIONS; round++) {
        // This _very roughly_ aims for REPETITIONS seconds at this many GHz.
        static constexpr size_t LOOPS{ 1 /* Giga */ * 1000000000 };

        const uint64_t tsc_before{ rdtsc() };

        for (size_t i = 0; i < LOOPS; i++) {
            // During this time, the VM can be paused and unpaused or similar.
            //
            // We cannot use PAUSE here, because its execution time is wildly
            // different for different processors and we are aiming for a
            // somewhat constant delay. Instead we use this empty assembler
            // statement to force the compiler to actually loop.
            asm volatile("");
        }

        const uint64_t tsc_after{ rdtsc() };

        // Be verbose for debugging.
        info("{}: TSC {} - {} = {}", round, tsc_after, tsc_before, tsc_after - tsc_before);

        BARETEST_ASSERT(tsc_before < tsc_after);
    }
}

BARETEST_RUN;
