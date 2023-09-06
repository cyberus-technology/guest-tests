/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <optional>

#include "idt.hpp"

#include <x86defs.hpp>

struct irqinfo
{
    using func_t = void (*)(intr_regs*);

    volatile bool valid {false};
    volatile uint8_t vec {0xff};
    volatile uint32_t err {~0u};
    volatile uintptr_t rip {~0ull};
    volatile func_t fixup_fn {nullptr};

    void reset() volatile
    {
        fixup_fn = nullptr;
        valid = false;
    }

    void record(uint8_t v, uint32_t e, uintptr_t ip = 0) volatile
    {
        valid = true;
        vec = v;
        err = e;
        rip = ip;
    }

    void fixup(intr_regs* regs) const volatile
    {
        if (fixup_fn) {
            fixup_fn(regs);
        }
    }
};
