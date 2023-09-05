/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <x86defs.hpp>

/* This is a RAII-Style class that disables the interrupts on creation and
 * enables them on destruction, if they have been enabled before.
 */

class irq_guard
{
public:
    irq_guard() : irq_enabled(check_if_irq_enabled()) { asm volatile("cli"); }

    ~irq_guard()
    {
        if (irq_enabled) {
            asm volatile("sti");
        }
    }

private:
    const bool irq_enabled;

    bool check_if_irq_enabled()
    {
        uint64_t irq_flag;
        asm volatile("pushf;"
                     "pop %0;"
                     : "=r"(irq_flag));
        return irq_flag & x86::FLAGS_IF;
    }
};
