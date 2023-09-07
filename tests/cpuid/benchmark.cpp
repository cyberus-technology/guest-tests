/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <cstdint>

#include <toyos/baretest/baretest.hpp>
#include <toyos/testhelper/statistics.hpp>
#include <toyos/x86/x86asm.hpp>

TEST_CASE(benchmark_cycles)
{
    const unsigned REPETITIONS {10000};

    // The default warm up rounds are not enough to get stable numbers.
    const unsigned WARM_UP_ROUNDS {10000};

    auto data {statistics::measure_cycles([]() { cpuid(1, 0); }, REPETITIONS, WARM_UP_ROUNDS)};

    BENCHMARK_RESULT("cpuid_cycles", data.avg(), "cycles");
}
