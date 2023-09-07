/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <toyos/x86/x86asm.hpp>

class cr0_guard
{
public:
    cr0_guard() : cr0_value(get_cr0()) {}

    ~cr0_guard() { set_cr0(cr0_value); }

private:
    const uint64_t cr0_value;
};
