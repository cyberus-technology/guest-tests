// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <toyos/x86/x86asm.hpp>

class cr0_guard
{
 public:
    cr0_guard()
        : cr0_value(get_cr0()) {}

    ~cr0_guard()
    {
        set_cr0(cr0_value);
    }

 private:
    const uint64_t cr0_value;
};
