#pragma once

#include <array>
#include <string>

#include <toyos/x86/cpuid.hpp>
#include <toyos/x86/x86asm.hpp>

namespace util::cpuid
{
    inline bool hv_bit_present()
    {
        return ::cpuid(CPUID_LEAF_FAMILY_FEATURES).ecx & LVL_0000_0001_ECX_HV;
    }

    /**
     * Returns the extended brand string from cpuid by reading all three
     * corresponding leaves and bringing the returned bytes into the
     * corresponding order.
     */
    inline std::string get_extended_brand_string()
    {
        // Amount of subsequent cpuid leaves to read.
        static constexpr size_t LEAF_COUNT = 3;
        // Amount of registers that hold the overall string.
        static constexpr size_t REGISTER_COUNT = 4 * LEAF_COUNT;

        // Holds the components of the string returned by cpuid.
        std::vector<uint32_t> brand_string_components{};
        brand_string_components.reserve(REGISTER_COUNT);

        for (unsigned long i = 0; i < LEAF_COUNT; i++) {
            auto res = ::cpuid(CPUID_LEAF_EXTENDED_BRAND_STRING_BASE + i);
            brand_string_components.push_back(res.eax);
            brand_string_components.push_back(res.ebx);
            brand_string_components.push_back(res.ecx);
            brand_string_components.push_back(res.edx);
        }

        // Just reinterpret as all bytes are already in a proper ASCII-order.
        return { reinterpret_cast<char*>(brand_string_components.data()) };
    }

}  // namespace util::cpuid
