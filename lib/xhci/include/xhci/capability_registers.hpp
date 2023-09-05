/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <compiler.hpp>
#include <math.hpp>

namespace xhci_capability_registers
{

struct __PACKED__ generic
{
    enum
    {
        OFFSET = 0x00,

        CAP_LENGTH_BITS = 8,
        CAP_LENGTH_SHIFT = 0,
    };

    uint8_t cap_length() const { return (raw >> CAP_LENGTH_SHIFT) & math::mask(CAP_LENGTH_BITS); }

    volatile uint32_t raw;
};

struct __PACKED__ hcsparams1
{
    enum
    {
        OFFSET = 0x04,

        SLOTS_BITS = 8,
        SLOTS_SHIFT = 0,
        INTRS_BITS = 11,
        INTRS_SHIFT = 8,
        PORTS_BITS = 8,
        PORTS_SHIFT = 24,
    };

    uint8_t max_slots() const { return (raw >> SLOTS_SHIFT) & math::mask(SLOTS_BITS); }
    uint8_t max_ports() const { return (raw >> PORTS_SHIFT) & math::mask(PORTS_BITS); }

    volatile uint32_t raw;
};

struct __PACKED__ hccparams1
{
    enum
    {
        OFFSET = 0x10,

        AC64 = 1u << 0,

        XECP_BITS = 16,
        XECP_SHIFT = 16,
    };

    bool supports_64bit_addressing() const { return raw & AC64; }

    // Note: The offset is provided in DWORD granularity!
    uint16_t ext_cap_offset_dwords() const { return (raw >> XECP_SHIFT) & math::mask(XECP_BITS); }

    volatile uint32_t raw;
};

} // namespace xhci_capability_registers
