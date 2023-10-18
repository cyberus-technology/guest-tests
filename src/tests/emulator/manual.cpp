/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <algorithm>
#include <array>

#include <toyos/baretest/baretest.hpp>
#include <toyos/testhelper/debugport_interface.h>

#define STOS(type, width)                                                                         \
    TEST_CASE(stos_##width)                                                                       \
    {                                                                                             \
        std::array<type, 64> output{}, compare{};                                                 \
        uintptr_t output_raw = reinterpret_cast<uintptr_t>(output.data());                        \
        asm volatile("outb %[dbgport];"                                                           \
                     "rep stos" #width " (%[output]);"                                            \
                     : [output] "+D"(output_raw)                                                  \
                     : "a"(DEBUGPORT_EMUL_ONCE), [dbgport] "i"(DEBUG_PORTS.a), "c"(output.size()) \
                     : "memory");                                                                 \
                                                                                                  \
        std::fill(compare.begin(), compare.end(), DEBUGPORT_EMUL_ONCE);                           \
        BARETEST_ASSERT((output == compare));                                                     \
    }

STOS(uint8_t, b);
#if UDIS_STOSW_FIXED
STOS(uint16_t, w);
#endif
STOS(uint32_t, l);
STOS(uint64_t, q);
