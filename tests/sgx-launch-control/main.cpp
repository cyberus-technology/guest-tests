/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <toyos/x86/cpuid.hpp>
#include <toyos/x86/x86asm.hpp>
#include <toyos/x86/x86defs.hpp>

#include <cbl/baretest/baretest.hpp>

using namespace x86;

TEST_CASE(sgx_feature_control_msr_reports_lc_availability)
{
    const auto feature_control_msr {rdmsr(msr::IA32_FEATURE_CONTROL)};
    const auto msr_sgx_lc_support {feature_control_msr & IA32_FEATURE_CONTROL_SGX_LAUNCH_CONTROL_ENABLE};

    BARETEST_ASSERT(msr_sgx_lc_support);
}

TEST_CASE(sgx_feature_control_cpuid_reports_availability)
{
    const auto cpuid_res {cpuid(CPUID_LEAF_EXTENDED_FEATURES)};
    const auto cpuid_lc_support {cpuid_res.ecx & LVL_0000_0007_ECX_SGX_LC};

    BARETEST_ASSERT(cpuid_lc_support);
}

TEST_CASE(sgx_launch_control_hash_msrs_are_writable)
{
    for (auto index : {msr::IA32_SGXLEPUBKEYHASH0, msr::IA32_SGXLEPUBKEYHASH1, msr::IA32_SGXLEPUBKEYHASH2,
                       msr::IA32_SGXLEPUBKEYHASH3}) {
        auto hash_read {rdmsr(index)};
        const auto hash_written {~hash_read};
        wrmsr(index, hash_written);
        hash_read = rdmsr(index);

        BARETEST_ASSERT(hash_read == hash_written);
    }
}

BARETEST_RUN;
