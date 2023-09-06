/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <initializer_list>
#include <tuple>

#include <debugport_interface.h>

#include <cbl/baretest/baretest.hpp>
#include <compiler.hpp>
#include <cpuid.hpp>

#include <x86asm.hpp>

template <typename SIZE_TYPE>
static void test_movbe_internal(SIZE_TYPE test_value)
{
    SIZE_TYPE dst_val {0}, dst_hw {0};

    asm volatile("movbe %[src], %[dst2];"
                 "outb %[dbgport];"
                 "movbe %[src], %[dst];"
                 : [dst] "=r"(dst_val), [dst2] "=&r"(dst_hw)
                 : [src] "m"(test_value), [dbgport] "i"(DEBUG_PORTS.a), "a"(DEBUGPORT_EMUL_ONCE));

    BARETEST_VERIFY(dst_val == dst_hw);
}

static bool has_movbe()
{
    return cpuid(CPUID_LEAF_FAMILY_FEATURES).ecx & LVL_0000_0001_ECX_MOVBE;
}

TEST_CASE_CONDITIONAL(movbe, has_movbe())
{
    test_movbe_internal<uint16_t>(0x1122);
    test_movbe_internal<uint32_t>(0x11223344);
    test_movbe_internal<uint64_t>(0xaabbccdd11223344);
}
