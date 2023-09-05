/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <compiler.hpp>
#include <decoder/gdt.hpp>

namespace emulator
{

struct msr_star
{
    enum mode
    {
        USER = 48,
        KERNEL = 32,
    };

    enum : uint16_t
    {
        AR_CS_KERNEL = 0xa9b, // G=1, L=1, DPL=0, accessed code segment
        AR_CS_USER32 = 0xcfb, // G=1, D=1, DPL=3, accessed code segment
        AR_CS_USER64 = 0xafb, // G=1, L=1, DPL=3, accessed code segment

        AR_SS_KERNEL = 0xc93, // G=1, D=1, DPL=0, accessed data segment
        AR_SS_USER = 0xcf3,   // G=1, D=1, DPL=3, accessed data segment
    };

    x86::gdt_segment cs(mode m, bool is_64bit = true)
    {
        uint8_t cpl = (m == mode::USER) ? 3 : 0;
        uint16_t sel_base = ((star >> m) & ~x86::SELECTOR_PL_MASK) | cpl;

        switch (m) {
        case mode::KERNEL: return {sel_base, AR_CS_KERNEL, ~0u, 0};
        case mode::USER: return {uint16_t(sel_base + 16u * is_64bit), is_64bit ? AR_CS_USER64 : AR_CS_USER32, ~0u, 0};
        }
        __UNREACHED__;
    }

    x86::gdt_segment ss(mode m)
    {
        uint8_t cpl = (m == mode::USER) ? 3 : 0;
        uint16_t sel = (((star >> m) & ~x86::SELECTOR_PL_MASK) + 8) | cpl;

        switch (m) {
        case mode::KERNEL: return {sel, AR_SS_KERNEL, ~0u, 0};
        case mode::USER: return {sel, AR_SS_USER, ~0u, 0};
        }
        __UNREACHED__;
    }

    uint64_t star;
};

} // namespace emulator
