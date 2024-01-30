/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <cassert>
#include <string>
#include <toyos/baretest/baretest.hpp>

TEST_CASE(string_pio_respects_address_override)
{
    uint64_t read_buffer_addr{ 0 };

    asm volatile(
        // Do a string port I/O with address override and DF=1. The Intel SDM implies that this will only use
        // 32-bit addresses.
        //
        // For simplicity, we set DF to count backwards from zero. Depending on how many bits flip, we see the
        // address size being used.
        //
        // Note: We rely on being able to read the zero address here. This works, because we run with identity
        // paging. If this ever changes, we will get a page fault on the outsb instruction and have to find a
        // different way to test this property.
        "std\n"
        "mov %%ecx, %%edx\n"
        ".byte 0x67; outsb\n"
        "cld\n"
        : "+S"(read_buffer_addr)
        :);

    // If the address override worked, we should only get a 32-bit wrap instead of a 64-bit one.
    BARETEST_ASSERT(read_buffer_addr == 0xFFFFFFFF);
}
