/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

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

TEST_CASE(boots_into_64bit_mode)
{
    printf("Hello from %s\n", __FUNCTION__);
}

TEST_CASE_CONDITIONAL(test_case_can_be_skipped, false)
{
    printf("Hello from %s\n", __FUNCTION__);
}
