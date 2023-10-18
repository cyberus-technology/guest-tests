/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <toyos/testhelper/lapic_test_tools.hpp>

using lvt_entry = lapic_test_tools::lvt_entry;

class lvt_guard
{
 public:
    lvt_guard(lvt_entry entry_, uint32_t vector, uint32_t mode)
        : entry(entry_)
    {
        write_lvt_entry(entry, { .vector = vector, .mode = mode, .mask = 0 });
    }

    ~lvt_guard()
    {
        write_lvt_mask(entry, 1);
    }

 private:
    lvt_entry entry;
};
