/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <initializer_list>
#include <tuple>

#include <toyos/testhelper/debugport_interface.h>

#include <cbl/baretest/baretest.hpp>
#include <compiler.hpp>


TEST_CASE(push_register)
{
    uint64_t test_value {0xdeadb33f}, test_value_compare {0xcafed00d};

    asm volatile("outb %[dbgport];"
                 "push %[src];"
                 "pop  %[dst];"
                 : [dst] "+r"(test_value_compare)
                 : [src] "r"(test_value), [dbgport] "i"(DEBUG_PORTS.a), "a"(DEBUGPORT_EMUL_ONCE));

    BARETEST_VERIFY(test_value == test_value_compare);
}

TEST_CASE(pop_register)
{
    uint64_t test_value {0xdeadb33f}, test_value_compare {0xcafed00d};

    asm volatile("push %[src];"
                 "outb %[dbgport];"
                 "pop  %[dst];"
                 : [dst] "+r"(test_value_compare)
                 : [src] "r"(test_value), [dbgport] "i"(DEBUG_PORTS.a), "a"(DEBUGPORT_EMUL_ONCE));

    BARETEST_VERIFY(test_value == test_value_compare);
}
