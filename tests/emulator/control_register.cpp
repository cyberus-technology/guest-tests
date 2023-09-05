/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <trace.hpp>

#include <baretest/baretest.hpp>
#include <lapic_test_tools.hpp>
#include <x86asm.hpp>
#include <x86defs.hpp>

using x86::cr0;

TEST_CASE(cr0_et_should_be_set_by_the_HW)
{
    auto cr0_before {get_cr0()};
    BARETEST_ASSERT(cr0_before & math::mask_from(cr0::ET));

    auto cr0_without_et {cr0_before & ~math::mask_from(cr0::ET)};
    set_cr0(cr0_without_et);

    BARETEST_ASSERT(get_cr0() == cr0_before);
}
