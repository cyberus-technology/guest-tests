// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <toyos/x86/cpuid.hpp>
#include <toyos/x86/x86asm.hpp>

inline bool ibrs_supported()
{
    return cpuid(CPUID_LEAF_EXTENDED_FEATURES).edx & LVL_0000_0007_EDX_IBRS_IBPB;
}
