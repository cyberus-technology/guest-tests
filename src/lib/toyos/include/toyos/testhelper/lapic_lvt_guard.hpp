// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <toyos/testhelper/lapic_test_tools.hpp>

using lvt_entry = lapic_test_tools::lvt_entry;
using lvt_timer_mode = lapic_test_tools::lvt_timer_mode;
using lvt_dlv_mode = lapic_test_tools::lvt_dlv_mode;
using lvt_entry_t = lapic_test_tools::lvt_entry_t;
using lvt_trigger_mode = lapic_test_tools::lvt_trigger_mode;
using lvt_pin_polarity = lapic_test_tools::lvt_pin_polarity;
using lvt_mask = lapic_test_tools::lvt_mask;

class lvt_guard
{
 public:
    lvt_guard(lvt_entry entry_, uint32_t vector, lvt_timer_mode mode)
        : entry(entry_)
    {
        write_lvt_entry(
            entry,
            lvt_entry_t::timer(
                vector,
                lvt_mask::UNMASKED,
                mode)

        );
    }

    ~lvt_guard()
    {
        write_lvt_mask(entry, lvt_mask::MASKED);
    }

 private:
    lvt_entry entry;
};
