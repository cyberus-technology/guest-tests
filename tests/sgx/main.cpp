/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <cpuid.hpp>
#include <x86asm.hpp>
#include <x86defs.hpp>

#include <baretest/baretest.hpp>

using namespace x86;

TEST_CASE(sgx_feature_control_msr_reports_availability)
{
    const auto feature_control_msr {rdmsr(msr::IA32_FEATURE_CONTROL)};
    const auto msr_sgx_support {feature_control_msr & IA32_FEATURE_CONTROL_SGX};

    BARETEST_ASSERT(msr_sgx_support);
}

TEST_CASE(sgx_cpuid_reports_availability)
{
    const auto cpuid_res {cpuid(CPUID_LEAF_EXTENDED_FEATURES)};
    const auto cpuid_sgx_support {cpuid_res.ebx & LVL_0000_0007_EBX_SGX};

    BARETEST_ASSERT(cpuid_sgx_support);
}

TEST_CASE(sgx_cpuid_enumeration_subleaf_works)
{
    // Use the SGX subleaf for reporting EPC sections.
    // There might be multiple EPC sections and system software should
    // repeat the query while increasing subleaf until an invalid
    // sub-leaf is found.
    // Since this test runs on SGX enabled hardware only,the first subleaf
    // is expected to report a valid section.
    static constexpr uint32_t SGX_SUBLEAF_EPC_ENUM_0 {0x2};

    const auto res {cpuid(CPUID_LEAF_SGX_CAPABILITY, SGX_SUBLEAF_EPC_ENUM_0)};
    const auto valid_epc_section_found {res.eax & 0x1};

    BARETEST_ASSERT(valid_epc_section_found);
}

BARETEST_RUN;
