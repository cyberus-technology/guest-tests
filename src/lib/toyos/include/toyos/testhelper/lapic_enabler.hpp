// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <toyos/testhelper/lapic_test_tools.hpp>

// A class that enables the LAPIC upon construction and restores it state upon destruction.
class lapic_enabler
{
    const bool lapic_was_enabled;

 public:
    lapic_enabler()
        : lapic_was_enabled(lapic_test_tools::software_apic_enabled())
    {
        lapic_test_tools::software_apic_enable();
    }

    ~lapic_enabler()
    {
        if (not lapic_was_enabled) {
            lapic_test_tools::software_apic_disable();
        }
    }
};
