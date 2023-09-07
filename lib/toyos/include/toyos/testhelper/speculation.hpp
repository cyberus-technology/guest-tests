/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <toyos/x86/cpuid.hpp>
#include <toyos/x86/x86asm.hpp>

inline bool ibrs_supported()
{
   return cpuid(CPUID_LEAF_EXTENDED_FEATURES).edx & LVL_0000_0007_EDX_IBRS_IBPB;
}
