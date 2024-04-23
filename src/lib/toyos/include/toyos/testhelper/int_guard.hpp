// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "../x86/x86asm.hpp"

class int_guard
{
 public:
    enum class irq_status
    {
        enabled = 1,
        enabled_and_halted = 2,
        disabled = 3,
    };

    int_guard(irq_status int_status = irq_status::disabled)
    {
        previously_enabled = interrupts_enabled();
        if (int_status == irq_status::enabled) {
            enable_interrupts();
        }
        else if (int_status == irq_status::enabled_and_halted) {
            enable_interrupts_and_halt();
        }
        else if (int_status == irq_status::disabled) {
            disable_interrupts();
        }
    }

    ~int_guard()
    {
        if (previously_enabled) {
            enable_interrupts();
        }
        else {
            disable_interrupts();
        }
    }

 private:
    bool previously_enabled;
};
