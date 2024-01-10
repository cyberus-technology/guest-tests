/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <array>
#include <cstring>
#include <string>

#include <toyos/baretest/baretest.hpp>
#include <toyos/x86/x86asm.hpp>

template<size_t N>
void test_cpuid_string(std::array<std::string, N>& valid_cpu_models)
{
    constexpr const auto CPU_MODEL_BYTES = 4 * 4 * 3;
    constexpr const auto CPUID_MODEL_LEAF = 0x80000002;

    char model[CPU_MODEL_BYTES];
    cpuid_parameter res = cpuid(CPUID_MODEL_LEAF);
    memset(model, 0, CPU_MODEL_BYTES);

    memcpy(model + 0, &res.eax, 4);
    memcpy(model + 4, &res.ebx, 4);
    memcpy(model + 8, &res.ecx, 4);
    memcpy(model + 12, &res.edx, 4);

    info("Valid CPU models:");
    for (auto& m : valid_cpu_models) {
        info(" * {s}", m.c_str());
    }
    info("Detected CPUID string {s}", model);

    bool found_valid_model = false;
    for (auto& m : valid_cpu_models) {
        found_valid_model = found_valid_model || (static_cast<char*>(model) == m);
    }

    BARETEST_ASSERT(found_valid_model);
}

std::array<std::string, 8> valid_cpu_models{ {
    "Intel(R) Core(TM",
    "      Intel(R) C",
    "Intel(R) Celeron",
    "Intel(R) Xeon(R)",
    "Intel(R) Pentium",
    "11th Gen Intel(R",
    "12th Gen Intel(R",
    "13th Gen Intel(R",
} };

TEST_CASE(cpuid_string_is_native)
{
    test_cpuid_string(valid_cpu_models);
}

BARETEST_RUN;
