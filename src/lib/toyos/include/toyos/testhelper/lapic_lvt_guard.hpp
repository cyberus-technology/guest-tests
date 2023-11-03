/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <toyos/testhelper/lapic_test_tools.hpp>

using lvt_entry = lapic_test_tools::lvt_entry;
using lvt_timer_mode = lapic_test_tools::lvt_timer_mode;
using lvt_dlv_mode = lapic_test_tools::lvt_dlv_mode;
using lvt_mask = lapic_test_tools::lvt_mask;

class lvt_guard
{
 public:
    lvt_guard(lvt_entry entry_, uint32_t vector, lvt_timer_mode mode)
        : entry(entry_)
    {
        write_lvt_entry(entry, { .vector = vector, .timer_mode = mode, .dlv_mode = lvt_dlv_mode::FIXED, .mask = lvt_mask::UNMASKED });
    }

    ~lvt_guard()
    {
        write_lvt_mask(entry, 1);
    }

 private:
    lvt_entry entry;
};
