/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "toyos/util/math.hpp"
#include <compiler.hpp>

struct __PACKED__ trb
{
    enum
    {
        CYCLE = 1u << 0,
        TOGGLE = 1u << 1,

        TYPE_BITS = 6,
        TYPE_SHIFT = 10,
        EP_ID_BITS = 5,
        EP_ID_SHIFT = 16,

        ENDPOINT_ID_IN = 3,
        ENDPOINT_ID_OUT = 2,

        ISP = 1u << 2,
        IOC = 1u << 5,
    };

    uint8_t type() const { return (control >> TYPE_SHIFT) & math::mask(TYPE_BITS); }
    void type(uint8_t val)
    {
        control &= ~math::mask(TYPE_BITS, TYPE_SHIFT);
        control |= (uint32_t(val) & math::mask(TYPE_BITS)) << TYPE_SHIFT;
    }

    uint8_t endpoint_id() const { return (control >> EP_ID_SHIFT) & math::mask(EP_ID_BITS); }

    bool cycle() const { return control & CYCLE; }
    void cycle(bool set)
    {
        control &= ~CYCLE;
        control |= set * CYCLE;
    }

    uint16_t length() const { return status; }
    void length(uint16_t len) { status = len; }
    void set_ioc() { control |= IOC; }
    void set_isp() { control |= ISP; }

    // To commit a TRB, the cycle bit is toggled,
    // after it has been set to "inactive" by the ring.
    void commit() { cycle(not cycle()); }

    bool toggle() const { return control & TOGGLE; }
    void toggle(bool set)
    {
        control &= ~TOGGLE;
        control |= set * TOGGLE;
    }

    volatile uint64_t buffer;
    volatile uint32_t status;
    volatile uint32_t control;
};
static_assert(sizeof(trb) == 16, "TRB must be 16 bytes in size!");

template <uint8_t TYPE_>
struct __PACKED__ trb_template : trb
{
    enum
    {
        TYPE = TYPE_,
    };
};

using trb_normal = trb_template<1>;
using trb_link = trb_template<6>;
using trb_transfer_event = trb_template<32>;
using trb_port_status_change_event = trb_template<34>;
