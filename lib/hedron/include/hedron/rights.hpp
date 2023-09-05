/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <stdint.h>

namespace hedron
{
class rights
{
public:
    enum
    {
        EMPTY = 0,
        FULL_ACCESS = 0x1f,
    };

    explicit rights(uint8_t val = 0) : raw(val) {}

    uint8_t value() const { return raw; }

    static rights full() { return rights(FULL_ACCESS); }
    bool empty() const { return raw == EMPTY; }

    bool operator==(const rights& o) const { return value() == o.value(); }

protected:
    uint8_t raw;
};

class io_rights : public rights
{
public:
    enum
    {
        ACCESSIBLE = 0x1,
    };

    explicit io_rights(bool make_accessible = false) : rights(ACCESSIBLE * make_accessible) {}
};

struct mem_rights : public rights
{
    enum
    {
        READ = 1u << 0,
        WRITE = 1u << 1,
        EXEC = 1u << 2,
    };

    explicit mem_rights(uint8_t val = 0) : rights(val) {}

    mem_rights operator-(const mem_rights& o) const { return mem_rights(raw & ~o.raw); }

    mem_rights& operator-=(const mem_rights& o)
    {
        raw &= ~o.raw;
        return *this;
    }

    static mem_rights full() { return mem_rights(READ | WRITE | EXEC); }
    static mem_rights none() { return mem_rights(0); }

    static mem_rights read() { return mem_rights(READ); }
    static mem_rights write() { return mem_rights(WRITE); }
    static mem_rights exec() { return mem_rights(EXEC); }

    static mem_rights rw() { return mem_rights(READ | WRITE); }
    static mem_rights rx() { return mem_rights(READ | EXEC); }

    mem_rights operator|(const mem_rights& o) const { return mem_rights {static_cast<uint8_t>(raw | o.raw)}; }

    mem_rights& operator|=(const mem_rights& o)
    {
        raw |= o.raw;
        return *this;
    }
};

struct pt_rights : public rights
{
    enum
    {
        CTRL = 1u << 0, //< pt_ctrl permitted if set
        CALL = 1u << 1, //< call permitted if set
    };

    explicit pt_rights(uint8_t val) : rights(val) {}
};

} // namespace hedron
