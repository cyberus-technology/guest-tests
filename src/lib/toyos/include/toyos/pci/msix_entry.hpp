/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <cstdint>

namespace pci
{

    /// Helper for an MSI-X table entry consisting of address, data, and control.
    struct msix_entry
    {
        /// Bit definitions.
        enum
        {
            IS_MASKED = 0x1,  ///< Indicates if this entry is masked or not.
        };

        /// Constructs an empty, masked entry.
        msix_entry()
            : msg_addr(0), msg_addr_hi(0), msg_data(0), ctrl(IS_MASKED) {}

        uint32_t msg_addr;     ///< Lower 32 address bits.
        uint32_t msg_addr_hi;  ///< Upper 32 address bits.
        uint32_t msg_data;     ///< Message data, encoding vector and delivery information.
        uint32_t ctrl;         ///< Control, hosting the mask bit.

        /// Returns if the entry is masked.
        bool masked() const volatile
        {
            return ctrl & IS_MASKED;
        }

        /// Returns the entry's complete 64-bit MSI address (the upper 32 bits might be 0).
        uint64_t msi_address() const volatile
        {
            return uint64_t(msg_addr_hi) << 32 | msg_addr;
        }

        /// Returns the entry's MSI data.
        uint64_t msi_data() const volatile
        {
            return msg_data;
        }

        void mask() volatile
        {
            ctrl |= IS_MASKED;
        }  ///< Masks this entry.
        void unmask() volatile
        {
            ctrl &= ~IS_MASKED;
        }  ///< Unmasks this entry.

        /// Sets the MSI address, breaking it down into upper and lower part.
        void msi_address(uint64_t addr) volatile
        {
            msg_addr = addr;
            msg_addr_hi = addr >> 32;
        }

        /// Sets the MSI data.
        void msi_data(uint64_t data) volatile
        {
            msg_data = data;
        }

        /// Compares this entry to o by comparing all four members sequentially.
        bool operator==(const msix_entry& o) const volatile
        {
            return msg_addr == o.msg_addr and msg_addr_hi == o.msg_addr_hi and msg_data == o.msg_data and ctrl == o.ctrl;
        }
    };
    static_assert(sizeof(msix_entry) == 16, "Wrong msix_entry layout!");

}  // namespace pci
