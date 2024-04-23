// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <array>
#include <cstdint>

#include "cpuid.hpp"

using xmm_t = std::array<uint64_t, 2>;
using ymm_t = std::array<uint64_t, 4>;
using zmm_t = std::array<uint64_t, 8>;

inline void set_mm0(uint64_t value)
{
    asm volatile("movq %0, %%mm0" ::"r"(value));
}

inline uint64_t get_mm0()
{
    uint64_t ret;
    asm volatile("movq %%mm0, %0"
                 : "=r"(ret));
    return ret;
}

inline void set_xmm0(const xmm_t& values)
{
    asm volatile("movdqu %0, %%xmm0" ::"m"(values));
}

inline auto get_xmm0()
{
    xmm_t ret;
    asm volatile("movdqu %%xmm0, %0"
                 : "=m"(ret));
    return ret;
}

inline void set_ymm0(const ymm_t& values)
{
    asm volatile("vmovdqu %0, %%ymm0" ::"m"(values));
}

inline auto get_ymm0()
{
    ymm_t ret;
    asm volatile("vmovdqu %%ymm0, %0"
                 : "=m"(ret));
    return ret;
}

inline void set_zmm0(const zmm_t& values)
{
    asm volatile("vmovdqu64 %0, %%zmm0" ::"m"(values));
}

inline auto get_zmm0()
{
    zmm_t ret;
    asm volatile("vmovdqu64 %%zmm0, %0"
                 : "=m"(ret));
    return ret;
}

inline void set_zmm23(const zmm_t& values)
{
    asm volatile("vmovdqu64 %0, %%zmm23" ::"m"(values));
}

inline auto get_zmm23()
{
    zmm_t ret;
    asm volatile("vmovdqu64 %%zmm23, %0"
                 : "=m"(ret));
    return ret;
}

inline void set_k0(uint64_t val)
{
    asm volatile("kmovq %0, %%k0" ::"rm"(val));
}

inline uint64_t get_k0()
{
    uint64_t ret;
    asm volatile("kmovq %%k0, %0"
                 : "=rm"(ret));
    return ret;
}

inline bool mmx_supported()
{
    return cpuid(CPUID_LEAF_FAMILY_FEATURES).edx & LVL_0000_0001_EDX_MMX;
}

inline bool sse_supported()
{
    return cpuid(CPUID_LEAF_FAMILY_FEATURES).edx & LVL_0000_0001_EDX_SSE;
}

inline bool sse2_supported()
{
    return cpuid(CPUID_LEAF_FAMILY_FEATURES).edx & LVL_0000_0001_EDX_SSE2;
}

inline bool xsave_supported()
{
    return cpuid(CPUID_LEAF_FAMILY_FEATURES).ecx & LVL_0000_0001_ECX_XSAVE;
}

inline bool osxsave_supported()
{
    return cpuid(CPUID_LEAF_FAMILY_FEATURES).ecx & LVL_0000_0001_ECX_OSXSAVE;
}

inline bool avx_supported()
{
    return cpuid(CPUID_LEAF_FAMILY_FEATURES).ecx & LVL_0000_0001_ECX_AVX;
}

inline bool avx512_supported()
{
    return cpuid(CPUID_LEAF_EXTENDED_FEATURES).ebx & LVL_0000_0007_EBX_AVX512F;
}

inline bool xsaveopt_supported()
{
    return cpuid(CPUID_LEAF_EXTENDED_STATE, CPUID_EXTENDED_STATE_SUB).eax & LVL_0000_000D_EAX_XSAVEOPT;
}

inline bool xsavec_supported()
{
    return cpuid(CPUID_LEAF_EXTENDED_STATE, CPUID_EXTENDED_STATE_SUB).eax & LVL_0000_000D_EAX_XSAVEC;
}

inline bool xsaves_supported()
{
    return cpuid(CPUID_LEAF_EXTENDED_STATE, CPUID_EXTENDED_STATE_SUB).eax & LVL_0000_000D_EAX_XSAVES;
}

inline void fxsave(uint8_t* store)
{
    asm volatile("fxsave %0"
                 : "=m"(*store)::"memory");
}

inline void fxrstor(uint8_t* store)
{
    asm volatile("fxrstor %0" ::"m"(*store)
                 : "memory");
}

#define XSAVE_VARIANT(mnemonic)                                                                                     \
    inline void mnemonic(uint8_t* store, uint64_t features)                                                         \
    {                                                                                                               \
        asm volatile(#mnemonic " %[area]" ::[area] "m"(*store), "a"(features & math::mask(32)), "d"(features >> 32) \
                     : "memory");                                                                                   \
    }

XSAVE_VARIANT(xsave)
XSAVE_VARIANT(xsaves)
XSAVE_VARIANT(xsavec)
XSAVE_VARIANT(xsaveopt)

inline void xrstor(uint8_t* store, uint64_t features)
{
    asm volatile("xrstor %[area]" ::"a"(features & math::mask(32)), "d"(features >> 32), [area] "m"(*store)
                 : "memory");
}

inline void xrstors(uint8_t* store, uint64_t features)
{
    asm volatile("xrstors %[area]" ::"a"(features & math::mask(32)), "d"(features >> 32), [area] "m"(*store)
                 : "memory");
}
