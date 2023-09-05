/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <arch.hpp>

namespace x86
{

uint32_t get_largest_extended_function_code()
{
    static const uint32_t function_code {cpuid(LARGEST_EXTENDED_FUNCTION_CODE).eax};
    return function_code;
}

uint32_t get_addr_size_info()
{
    PANIC_UNLESS(get_largest_extended_function_code() >= ADDR_SIZE_INFORMATION, "address size info not available");
    static const uint32_t addr_size {cpuid(ADDR_SIZE_INFORMATION).eax};
    return addr_size;
}

uint64_t get_max_phy_addr_bits()
{
    uint32_t info {get_addr_size_info()};
    return (info & PHY_ADDR_BITS_MASK) >> PHY_ADDR_BITS_SHIFT;
}

uint64_t get_max_lin_addr_bits()
{
    uint32_t info {get_addr_size_info()};
    return (info & LIN_ADDR_BITS_MASK) >> LIN_ADDR_BITS_SHIFT;
}

} // namespace x86
