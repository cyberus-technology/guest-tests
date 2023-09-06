/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <cbl/traits.hpp>
#include "cpuid.hpp"
#include "cbl/math.hpp"
#include "segmentation.hpp"
#include "x86defs.hpp"

#define CONCAT3(A, B, C) A##B##C

#define WR_SEGMENT_BASE(seg)                                                                                           \
    inline void CONCAT3(wr, seg, base)(uint64_t value)                                                                 \
    {                                                                                                                  \
        asm volatile("wr" #seg "base %0" ::"r"(value));                                                                \
    }

#define RD_SEGMENT_BASE(seg)                                                                                           \
    inline uint64_t CONCAT3(rd, seg, base)()                                                                           \
    {                                                                                                                  \
        uint64_t value;                                                                                                \
        asm volatile("rd" #seg "base %0" : "=r"(value));                                                               \
        return value;                                                                                                  \
    }

WR_SEGMENT_BASE(fs)
RD_SEGMENT_BASE(fs)

WR_SEGMENT_BASE(gs)
RD_SEGMENT_BASE(gs)

inline uint8_t inb(uint16_t port)
{
    unsigned value;
    asm volatile("inb %%dx, %%al" : "=a"(value) : "d"(port));
    return value;
}

inline void outb(uint16_t port, uint8_t value)
{
    asm volatile("outb %%al, %%dx" ::"d"(port), "a"(value));
}

inline uint16_t inw(uint16_t port)
{
    unsigned value;
    asm volatile("inw %%dx, %%ax" : "=a"(value) : "d"(port));
    return value;
}

inline void outw(uint16_t port, uint16_t value)
{
    asm volatile("outw %%ax, %%dx" ::"d"(port), "a"(value));
}

inline uint64_t rdtsc()
{
    uint32_t hi, lo;
    asm volatile("rdtsc" : "=d"(hi), "=a"(lo));
    return (static_cast<uint64_t>(hi) << cbl::bit_width(hi)) | lo;
}

inline uint64_t rdtscp()
{
    uint32_t hi, lo;
    [[maybe_unused]] uint32_t tsc_aux;
    asm volatile("rdtscp" : "=d"(hi), "=a"(lo), "=c"(tsc_aux));
    return (static_cast<uint64_t>(hi) << cbl::bit_width(hi)) | lo;
}

inline uint32_t get_tsc_aux()
{
    uint32_t hi, lo;
    uint32_t tsc_aux;
    asm volatile("rdtscp" : "=d"(hi), "=a"(lo), "=c"(tsc_aux));
    return tsc_aux;
}

inline void cpu_pause()
{
    asm volatile("pause");
}

struct cpuid_parameter
{
    uint32_t eax, ebx, ecx, edx;
};

inline cpuid_parameter cpuid(uint32_t leaf, uint32_t subleaf = 0)
{
    cpuid_parameter result {leaf, 0, subleaf, 0};
    asm volatile("cpuid" : "+a"(result.eax), "=b"(result.ebx), "+c"(result.ecx), "=d"(result.edx));
    return result;
}

inline uint64_t rdmsr(uint32_t idx)
{
    uint32_t val_low, val_high;
    asm volatile("rdmsr" : "=d"(val_high), "=a"(val_low) : "c"(idx));
    return static_cast<uint64_t>(val_high) << 32 | val_low;
}

inline void wrmsr(uint32_t idx, uint64_t value)
{
    asm volatile("wrmsr" ::"c"(idx), "d"(value >> 32), "a"(value));
}

inline x86::descriptor_ptr get_current_gdtr()
{
    x86::descriptor_ptr ret;
    asm volatile("sgdt %0" : "=m"(ret));
    return ret;
}

inline x86::descriptor_ptr get_current_idtr()
{
    x86::descriptor_ptr ret;
    asm volatile("sidt %0" : "=m"(ret));
    return ret;
}

#define SET_CR(crN)                                                                                                    \
    inline void set_cr##crN(uint64_t val)                                                                              \
    {                                                                                                                  \
        asm volatile("mov %0, %%cr" #crN ::"r"(val));                                                                  \
    }

#define GET_CR(crN)                                                                                                    \
    inline uint64_t get_cr##crN()                                                                                      \
    {                                                                                                                  \
        uint64_t ret;                                                                                                  \
        asm volatile("mov %%cr" #crN ", %0" : "=r"(ret));                                                              \
        return ret;                                                                                                    \
    }

SET_CR(0)
SET_CR(2)
SET_CR(3)
SET_CR(4)
SET_CR(8)

GET_CR(0)
GET_CR(2)
GET_CR(3)
GET_CR(4)
GET_CR(8)

inline uint64_t get_xcr(uint32_t xcrN = 0)
{
    uint32_t low, high;
    asm volatile("xgetbv" : "=a"(low), "=d"(high) : "c"(xcrN));
    return static_cast<uint64_t>(high) << 32 | low;
}

inline void set_xcr(uint64_t val, uint32_t xcrN = 0)
{
    asm volatile("xsetbv" ::"a"(val & math::mask(32)), "d"(val >> 32), "c"(xcrN));
}

inline void invlpg(uintptr_t lin_addr)
{
    asm volatile("invlpg (%0)" ::"r"(lin_addr) : "memory");
}

inline bool interrupts_enabled()
{
    uint64_t flags;
    asm volatile("pushf;"
                 "pop %0;"
                 : "=r"(flags)
                 :
                 : "memory");
    return flags & x86::FLAGS_IF;
}

inline void enable_interrupts()
{
    asm volatile("sti;");
}

inline void enable_interrupts_and_halt()
{
    asm volatile("sti; hlt;");
}

inline void enable_interrupts_for_single_instruction()
{
    asm volatile("sti; nop; cli;");
}

inline void disable_interrupts()
{
    asm volatile("cli;");
}

inline void disable_interrupts_and_halt()
{
    asm volatile("cli; hlt;");
}

#define GET_SEGMENT_SELECTOR(seg)                                                                                      \
    inline uint16_t get_##seg()                                                                                        \
    {                                                                                                                  \
        uint16_t ret;                                                                                                  \
        asm volatile("mov %%" #seg ", %0" : "=r"(ret));                                                                \
        return ret;                                                                                                    \
    }

GET_SEGMENT_SELECTOR(ss)
GET_SEGMENT_SELECTOR(cs)
GET_SEGMENT_SELECTOR(ds)
GET_SEGMENT_SELECTOR(es)
GET_SEGMENT_SELECTOR(fs)
GET_SEGMENT_SELECTOR(gs)

// Clashes with a function in one of our guest tests and has to use a namespace.
// All other functions should be moved to the namespace as well (see ticket #495).
namespace x86
{

inline uint16_t sldt()
{
    uint16_t ret;
    asm volatile("sldt %0" : "=m"(ret));
    return ret;
}

inline uint16_t str()
{
    uint16_t ret;
    asm volatile("str %0" : "=m"(ret));
    return ret;
}

template <decltype(cpuid) CPUID_FN>
inline cpu_info get_cpu_info_internal()
{
    // See Intel SDM, Vol. 2, CPUID instruction for more details.

    const auto infos {CPUID_FN(CPUID_LEAF_FAMILY_FEATURES, 0).eax};
    const auto stepping {infos & math::mask(4)};
    const auto model {(infos >> 4) & math::mask(4)};
    const auto family {(infos >> 8) & math::mask(4)};

    const auto extended_model {(infos >> 16) & math::mask(4)};
    const auto extended_family {(infos >> 20) & math::mask(8)};

    return {
        .family = family + ((family == 0xf) ? extended_family : 0),
        .model = model | ((family == 0x6 or family == 0xf) ? extended_model << 4 : 0),
        .stepping = stepping,
    };
}

inline cpu_info get_cpu_info()
{
    return get_cpu_info_internal<cpuid>();
}

} // namespace x86

inline uint64_t get_rflags()
{
    uint64_t ret;
    asm volatile("pushfq; pop %0;" : "=r"(ret) : : "memory");
    return ret;
}

template <typename MODIFY_FN>
inline void modify_rflags(MODIFY_FN mod)
{
    uint64_t rflags {mod(get_rflags())};

    asm volatile("push %0 ; popf" ::"rm"(rflags));
}

inline void set_rflags(uint64_t flags)
{
    modify_rflags([flags](uint64_t rflags) { return rflags | flags; });
}

inline void clear_rflags(uint64_t flags)
{
    modify_rflags([flags](uint64_t rflags) { return rflags & ~flags; });
}
