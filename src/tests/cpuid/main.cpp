/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <array>
#include <cstring>
#include <string>

#include <toyos/baretest/baretest.hpp>
#include <toyos/util/cpuid.hpp>

template<size_t N>
void test_cpuid_string(std::array<std::string, N>& valid_cpu_models)
{
    auto model = util::cpuid::get_extended_brand_string();

    info("Valid CPU models:");
    for (auto& m : valid_cpu_models) {
        info(" * {s}", m.c_str());
    }
    info("Detected CPUID string \"{s}\"", model.c_str());

    bool found_valid_model = false;
    for (auto& m : valid_cpu_models) {
        if (m == model.substr(0, 16)) {
            found_valid_model = true;
            break;
        }
    }

    BARETEST_ASSERT(found_valid_model);
}

/*
 * This list is only chosen to match our own CI testing infrastructure. We can
 * and should extend it in the future.
 *
 * Keep in sync with README!
 */
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
