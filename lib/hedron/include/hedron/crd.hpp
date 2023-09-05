/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <hedron/rights.hpp>

namespace hedron
{
class crd
{
public:
    enum type : uint64_t
    {
        NULL_CAP = 0,
        MEM_CAP,
        IO_CAP,
        OBJ_CAP,
    };

    enum masks : uint64_t
    {
        TYPE_MASK = 0x3,
        RIGHT_MASK = 0x1f,
        ORDER_MASK = 0x1f,
    };

    enum shifts : uint64_t
    {
        TYPE_SHIFT = 0,
        RIGHT_SHIFT = 2,
        ORDER_SHIFT = 7,
        BASE_SHIFT = 12,
    };

    uint64_t value() const { return raw; }
    uint64_t base() const { return raw >> BASE_SHIFT; }
    uint64_t order() const { return (raw >> ORDER_SHIFT) & ORDER_MASK; }
    type get_type() const { return static_cast<type>((raw >> TYPE_SHIFT) & TYPE_MASK); }

    crd() = default;

    explicit crd(uint64_t r) : raw(r) {}

    crd(type t, rights r, uint64_t b, uint8_t o)
    {
        o &= ORDER_MASK;
        raw = (t << TYPE_SHIFT) | (r.value() << RIGHT_SHIFT) | (o << ORDER_SHIFT) | (b << BASE_SHIFT);
    }

private:
    uint64_t raw {0};
};
} // namespace hedron
