// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "toyos/util/math.hpp"

#include <cstdint>

namespace pci_internal
{

    using read_fn_t = uint32_t(const uint32_t*);
    using write_fn_t = void(uint32_t*, uint32_t);

    inline uint32_t default_read_fn(const uint32_t* addr)
    {
        return *const_cast<const volatile uint32_t*>(addr);
    }

    inline void default_write_fn(uint32_t* addr, uint32_t value)
    {
        *const_cast<volatile uint32_t*>(addr) = value;
    }

    /**
 * PCI BAR structure
 *
 * This abstraction provides functionality to determine the type and region of a
 * given BAR, as well as calculating the size and changing the address.
 *
 * \tparam READ_FN Function to use for reading memory belonging to the BAR
 * \tparam WRITE_FN Function to use for writing memory belonging to the BAR
 */
    template<read_fn_t READ_FN, write_fn_t WRITE_FN>
    struct bar_t_internal
    {
        /// BAR type (memory or I/O)
        enum class type
        {
            MEM = 0,  ///< Memory BAR.
            IO = 1,   ///< I/O BAR.
        };

        /// BAR width (32- or 64-bit)
        enum class width
        {
            BIT32 = 0,  ///< 32-bit BAR.
            BIT64 = 2,  ///< 64-bit BAR.
        };

        enum
        {
            TYPE_MASK = 0x1,                ///< Mask for the type indicator.
            WIDTH_BITS = 2,                 ///< Number of bits for the 32/64-bit indicator.
            WIDTH_SHIFT = 1,                ///< Shift for the 32/64-bit indicator.
            ADDRESS_MASK = ~math::mask(4),  ///< Mask for getting the actual address.
        };

        /// Constructs an instance with the given raw values.
        bar_t_internal(uint32_t low, uint32_t high)
            : raw(low), raw_hi(high) {}

        /// Returns if the BAR is 64-bit.
        bool is_64bit()
        {
            return width(((static_cast<uint32_t>(raw) >> WIDTH_SHIFT) & math::mask(WIDTH_BITS))) == width::BIT64;
        }

        /// Returns if the BAR is a memory BAR.
        bool is_mem()
        {
            return type((static_cast<uint32_t>(raw) & TYPE_MASK)) == type::MEM;
        }

        /// Returns if the BAR is an I/O BAR.
        bool is_pio()
        {
            return type((static_cast<uint32_t>(raw) & TYPE_MASK)) == type::IO;
        }

        /// Returns the complete address, honoring 32/64-bit and the address mask.
        uint64_t address()
        {
            return (static_cast<uint32_t>(raw) & ADDRESS_MASK)
                   | (is_64bit() ? static_cast<uint64_t>(static_cast<uint32_t>(raw_hi)) << 32 : 0);
        }

        /// Sets the address, honoring 32/64-bit and the address mask.
        void address(uint64_t val)
        {
            auto new_raw = static_cast<uint32_t>(raw) & ~ADDRESS_MASK;
            new_raw |= static_cast<uint32_t>(val) & ADDRESS_MASK;
            raw = new_raw;

            if (is_64bit()) {
                raw_hi = static_cast<uint32_t>(val >> 32);
            }
        }

        /**
     * Determines the size of the BAR.
     *
     * Note that this causes configuration space accesses.
     */
        uint64_t bar_size()
        {
            // The algorithm specified by the PCI Local Bus Specification is the
            // following:
            // 1. Save original value
            // 2. Write all 1's
            // 3. Read back new value
            // 4. Clear non-address bits, negate, increment by 1 to obtain size
            // 5. Restore original value
            //
            // Source: https://www.ics.uci.edu/~harris/ics216/pci/PCI_22.pdf
            auto orig_val = address();

            address(math::mask(64));
            auto new_val = address();

            // Some devices apparently don't toggle all upper bits,
            // so we use the resulting value as a mask.
            auto size = (~new_val + 1) & new_val;

            address(orig_val);
            return size;
        }

     private:
        // Proxy structure allowing to channel all accesses to the BAR register set
        // through a set of read/write functions. This is useful for cases where the
        // code should not touch memory directly.
        struct access_proxy
        {
            access_proxy(uint32_t val)
            {
                WRITE_FN(&value, val);
            }

            explicit constexpr operator uint32_t() const
            {
                return READ_FN(&value);
            }
            access_proxy& operator=(const uint32_t& val)
            {
                WRITE_FN(&value, val);
                return *this;
            }

            uint32_t value;
        };

        access_proxy raw;
        access_proxy raw_hi;
    };

}  // namespace pci_internal

namespace pci
{

    using bar_t = pci_internal::bar_t_internal<pci_internal::default_read_fn, pci_internal::default_write_fn>;
    static_assert(sizeof(bar_t) == 8, "Wrong BAR layout!");

}  // namespace pci
