/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "msix_entry.hpp"

#include "toyos/util/math.hpp"

#include <cstdint>

namespace pci
{
/// PCI capability base definition (consisting of capability ID and next pointer).
#pragma pack(push, 1)
struct capability
{
    uint8_t id;   ///< The PCI capability ID.
    uint8_t next; ///< The offset to the next capability structure.
};

struct ext_capability
{
    uint16_t id;
    uint16_t version_and_next;
};

/// Null capability with no functionality other than pointing to the next capability.
struct null_capability : capability
{
    enum
    {
        ID = 0
    };
};

/// PCI Express capability which is used to identify whether the extended config space is available.
struct pcie_capability : capability
{
    enum
    {
        ID = 0x10
    };
};

/// MSI capability specialization.
struct msi_capability : capability
{
    enum
    {
        ID = 5,                        ///< MSI capability ID.
        IS_ENABLED = 0x1,              ///< Enabled/Disabled bit.
        ORDER_MASK = 0x70,             ///< Mask for order bits in control field.
        ORDER_SHIFT = 4,               ///< Shift for order bits in control field.
        IS_64B_CAPABLE = 0x80,         ///< 64-bit indicator in control field.
        IS_PERVECTOR_MASKABLE = 0x100, ///< Per-vector masking indicator in control field.
    };

    uint16_t msg_ctrl; ///< Control field for enabling/disabling and vector count configuration.
    uint32_t msg_addr; ///< MSI address field (lower 32 bits).

    union
    {
        uint16_t msg_data; ///< MSI data field for 32-bit variant.
        struct
        {
            uint32_t msg_addr_hi;  ///< MSI address field (upper 32 bits for 64-bit variant).
            uint16_t msg_data_64b; ///< MSI data field for 64-bit variant.
        };
    };

    /// Returns the size of the capability structure depending on 32/64-bit.
    size_t size() const volatile { return is_64bit() ? 14 : 10; }

    /// Returns if the cap is 64-bit capable.
    bool is_64bit() const volatile { return msg_ctrl & IS_64B_CAPABLE; }

    /// Returns if individual vectors of a multi-message MSI can be masked.
    bool is_pervector_maskable() const volatile { return msg_ctrl & IS_PERVECTOR_MASKABLE; }

    /// Returns if the MSI capability is enabled.
    bool enabled() const volatile { return msg_ctrl & IS_ENABLED; }

    /// Returns the number of bits that comprise the multi-message MSI range.
    uint8_t order() const volatile { return (msg_ctrl & ORDER_MASK) >> ORDER_SHIFT; }

    /// Returns the complete MSI address, honoring 32/64-bit.
    uint64_t msi_addr() const volatile { return is_64bit() ? (uint64_t(msg_addr_hi) << 32) | msg_addr : msg_addr; }

    /// Returns the MSI data field, honoring 32/64-bit.
    uint16_t msi_data() const volatile { return is_64bit() ? msg_data_64b : msg_data; }

    /// Enables the MSI capability.
    void enable() volatile { msg_ctrl |= IS_ENABLED; }

    /// Disables the MSI capability.
    void disable() volatile { msg_ctrl &= ~IS_ENABLED; }

    /// Configures the number of bits that comprise the multi-message MSI range.
    void order(uint8_t order) volatile
    {
        msg_ctrl &= ~ORDER_MASK;
        msg_ctrl |= (order << ORDER_SHIFT);
    }

    /// Sets the MSI address, honoring 32/64-bit.
    void msi_addr(uint64_t addr) volatile
    {
        msg_addr = addr;
        if (is_64bit()) {
            msg_addr_hi = addr >> 32;
        }
    }

    /// Sets the MSI data, honoring 32/64-bit.
    void msi_data(uint64_t data) volatile
    {
        if (is_64bit()) {
            msg_data_64b = data;
        } else {
            msg_data = data;
        }
    }

    /// Copies rhs into this instance.
    void operator=(const volatile msi_capability& rhs) volatile
    {
        id = rhs.id;
        next = rhs.next;
        msg_ctrl = rhs.msg_ctrl;
        msg_addr = rhs.msg_addr;

        if (rhs.is_64bit()) {
            msg_addr_hi = rhs.msg_addr_hi;
            msg_data_64b = rhs.msg_data_64b;
        } else {
            msg_data = rhs.msg_data;
        }
    }

    // We need an explicit constructors here, because we declare an assignment operator above. Otherwise, we trigger
    // compiler warnings.
    msi_capability(const msi_capability& other) { (*this) = other; }

    msi_capability() = default;
};

/// MSI-X capability specialization.
struct msix_capability : capability
{
    enum
    {
        ID = 0x11,                      ///< MSI-X capability ID
        IS_ENABLED = 1 << 15,           ///< Enabled/Disabled bit in control field.
        BAR_MASK = math::mask(3),       ///< Mask for BAR number in control field.
        TBL_SIZE_MASK = math::mask(11), ///< Mask for MSI-X table size in control field.
    };

    /// Returns if the capability is enabled.
    bool enabled() const volatile { return msg_ctrl & IS_ENABLED; }

    /// Returns the BAR number hosting the MSI-X table.
    uint8_t table_bar() const volatile { return offset_table & BAR_MASK; }

    /// Returns the offset at which the MSI-X table resides within the BAR.
    size_t table_offset() const volatile { return offset_table & ~BAR_MASK; }

    /// Returns the number of entries in the MSI-X table.
    size_t table_entries() const volatile { return (msg_ctrl & TBL_SIZE_MASK) + 1; }

    /// Returns the size of the MSI-X table in bytes.
    size_t table_size() const volatile { return table_entries() * sizeof(msix_entry); }

    void enable() volatile { msg_ctrl |= IS_ENABLED; }   ///< Enables the MSI-X capability.
    void disable() volatile { msg_ctrl &= ~IS_ENABLED; } ///< Disables the MSI-X capability.

    /// Copies rhs into this instance.
    void operator=(volatile msix_capability& rhs) volatile
    {
        id = rhs.id;
        next = rhs.next;
        msg_ctrl = rhs.msg_ctrl;
        offset_table = rhs.offset_table;
        offset_pba = rhs.offset_pba;
    }

    uint16_t msg_ctrl;     ///< Control field for enabling/disabling and BAR/table size information.
    uint32_t offset_table; ///< MSI-X table offset in given BAR.
    uint32_t offset_pba;   ///< MSI-X pending bit array offset in the same BAR.
};

struct sriov_capability : ext_capability
{
    enum
    {
        ID = 0x10,           ///< SR-IOV capability ID.
        VF_ENABLED = 1 << 0, ///< Enabled field in SRIOV-Control
    };

    bool vf_enabled() const { return sriov_control & VF_ENABLED; }

    uint32_t sriov_capabilities;
    uint16_t sriov_control;
    uint16_t sriov_status;
    uint16_t initial_vfs;
    uint16_t total_vfs;
    uint16_t num_vfs;
    uint8_t function_dependency_link;
    uint8_t reserved1;
    uint16_t vf_offset;
    uint16_t vf_stride;
    uint16_t reserved2;
    uint16_t vf_device_id;
    uint32_t supported_page_sizes;
    uint32_t system_page_size;
    uint32_t vf_bar[6];
    uint32_t vf_migration_state_array;
};
#pragma pack(pop)
} // namespace pci
