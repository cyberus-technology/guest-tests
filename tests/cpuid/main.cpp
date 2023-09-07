/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <algorithm>
#include <array>
#include <cstring>
#include <string>
#include <vector>

#include <toyos/x86/cpuid.hpp>
#include <toyos/testhelper/debugport_interface.h>
#include <toyos/x86/x86asm.hpp>
#include <toyos/x86/x86fpu.hpp>

#include <toyos/baretest/baretest.hpp>

template <size_t N>
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

std::array<std::string, 8> valid_cpu_models {{
    "Intel(R) Core(TM",
    "      Intel(R) C",
    "Intel(R) Celeron",
    "Intel(R) Xeon(R)",
    "Intel(R) Pentium",
    "11th Gen Intel(R",
    "12th Gen Intel(R",
    "13th Gen Intel(R",
}};

std::array<std::string, 1> running_on_supernova {{">>> Running on S"}};

#ifdef RELEASE
TEST_CASE(cpuid_string_is_native)
{
    test_cpuid_string(valid_cpu_models);
}
#else
TEST_CASE_CONDITIONAL(cpuid_string_is_native, not debugport_present())
{
    test_cpuid_string(valid_cpu_models);
}

TEST_CASE_CONDITIONAL(cpuid_string_is_supernova, debugport_present())
{
    test_cpuid_string(running_on_supernova);
}
#endif

TEST_CASE_CONDITIONAL(xss_feature_is_not_available, debugport_present() and xsave_supported())
{
    // According to SDM 2, XSAVES instruction, the corresponding feature bit
    // is CPUID.(EAX=0DH,ECX=1):EAX.XSS[bit 3].
    // We require this leaf/subleaf combination to return 0.
    BARETEST_ASSERT((cpuid(CPUID_LEAF_EXTENDED_STATE, CPUID_EXTENDED_STATE_SUB).eax & LVL_0000_000D_EAX_XSAVES) == 0);
}

#if 0 /* disabled until hedron supports x2apic, see hedron#212 */
TEST_CASE_CONDITIONAL(x2apic_feature_is_available, debugport_present())
{
    static constexpr uint32_t X2APIC_LEAF {1};
    auto res = cpuid(X2APIC_LEAF);
    BARETEST_ASSERT(res.ecx & LVL_0000_0001_ECX_X2APIC);
}
#endif

BARETEST_RUN;
