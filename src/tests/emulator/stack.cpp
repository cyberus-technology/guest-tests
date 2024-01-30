/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <toyos/baretest/baretest.hpp>

TEST_CASE(push_register)
{
    uint64_t test_value{ 0xdeadb33f }, test_value_compare{ 0xcafed00d };

    asm volatile("push %[src];"
                 "pop  %[dst];"
                 : [dst] "+r"(test_value_compare)
                 : [src] "r"(test_value));

    BARETEST_VERIFY(test_value == test_value_compare);
}

TEST_CASE(pop_register)
{
    uint64_t test_value{ 0xdeadb33f }, test_value_compare{ 0xcafed00d };

    asm volatile("push %[src];"
                 "pop  %[dst];"
                 : [dst] "+r"(test_value_compare)
                 : [src] "r"(test_value));

    BARETEST_VERIFY(test_value == test_value_compare);
}
