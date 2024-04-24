// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <toyos/baretest/baretest.hpp>

BARETEST_RUN;

void prologue()
{
    printf("Hello from %s\n", __FUNCTION__);
}

void epilogue()
{
    printf("Hello from %s\n", __FUNCTION__);
}

TEST_CASE(boots_into_64bit_mode_and_runs_test_case)
{
    printf("Hello from %s\n", __FUNCTION__);
}

TEST_CASE_CONDITIONAL(test_case_can_be_skipped, false)
{
    BARETEST_ASSERT(false);
}

TEST_CASE(test_case_is_skipped_by_cmdline)
{
    BARETEST_ASSERT(false);
}
