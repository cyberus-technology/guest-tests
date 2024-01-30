/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <algorithm>
#include <array>

#include <toyos/baretest/baretest.hpp>

#define STOS(type, width)                                                  \
    TEST_CASE(stos_##width)                                                \
    {                                                                      \
        std::array<type, 64> output{}, compare{};                          \
        uintptr_t output_raw = reinterpret_cast<uintptr_t>(output.data()); \
        asm volatile("rep stos" #width " (%[output]);"                     \
                     : [output] "+D"(output_raw)                           \
                     : "c"(output.size())                                  \
                     : "memory");                                          \
                                                                           \
        std::fill(compare.begin(), compare.end(), 0x0);                    \
        BARETEST_ASSERT((output == compare));                              \
    }

STOS(uint8_t, b);
STOS(uint32_t, l);
STOS(uint64_t, q);
