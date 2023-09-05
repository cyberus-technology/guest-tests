/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <baretest/baretest.hpp>
#include <cassert>
#include <debugport_interface.h>
#include <string>
#include <trace.hpp>

TEST_CASE(string_pio_respects_address_override)
{
    // This is the SuperNOVA debug port, which will trigger the emulation path or the system's POST port, which can
    // handle receiving random data.
    uint64_t TEST_PORT {DEBUGPORT_NUMBER};
    uint64_t read_buffer_addr {0};

    // If this assertion fails, we need to adapt the assembly constraints, which need the test port to be in RDX.
    assert(std::string(DEBUGPORT_RETVAL_REG_ASM) == "rdx");

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
        : "+S"(read_buffer_addr),
          // This register is overwritten with the result of the debug port call, when running on SuperNOVA.
          "+d"(TEST_PORT)
        : "a"(DEBUGPORT_QUERY_HV));

    // If the address override worked, we should only get a 32-bit wrap instead of a 64-bit one.
    BARETEST_ASSERT(read_buffer_addr == 0xFFFFFFFF);
}
