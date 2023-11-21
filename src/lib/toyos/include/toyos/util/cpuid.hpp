#pragma once

#include <toyos/x86/cpuid.hpp>
#include <toyos/x86/x86asm.hpp>

static inline bool hv_bit_present()
{
    return cpuid(CPUID_LEAF_FAMILY_FEATURES).ecx & LVL_0000_0001_ECX_HV;
}
