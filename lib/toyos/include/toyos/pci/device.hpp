/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "bar.hpp"
#include "bdf.hpp"
#include "capabilities.hpp"
#include "msix_entry.hpp"
#include "toyos/pci/pci.hpp"

#include <toyos/x86/arch.hpp>
#include "toyos/util/math.hpp"

#include <toyos/util/interval.hpp>

#include <cstdint>

/**
 * PCI Device structure
 *
 * This class encapsulates PCI device identification
 * and properties like BARs and capabilities.
 */
class pci_device
{
    enum class shifts
    {
        VENDOR_ID = 0,
        DEVICE_ID = 16,
        DEV_TYPE = 16,
        CLASS = 24,
        SUBCLASS = 16,
        PROG_IF = 8,

        BUS_PRIMARY = 0,
        BUS_SECONDARY = 8,
        BUS_SUBORDINATE = 16,
    };

    enum masks
    {
        DEV_TYPE = math::mask(7),
    };

public:
    using bdf_t = pci::bdf_t; ///< Alias to the BDF structure.

    /// DWORD Offsets in the MMCONFIG space.
    enum class offset
    {
        DEVICE_VENDOR_ID = 0x00, ///< Device and Vendor identifier.
        CLASS = 0x08,            ///< Device class.
        BAR = 0x10,              ///< First BAR.
        HEADER_TYPE = 0x0c,      ///< PCI header type.
        CAP_PTR = 0x34,          ///< Pointer to first capability.

        BUS_INFO = 0x18, ///< Primary, secondary, subordinate bus numbers
    };

    /**
     * Constructs a device for the given MMCONFIG address and BDF.
     * \param cfg_base The start address of the device's memory-mapped config space.
     * \param bdf      The BDF describing the location on the bus.
     */
    explicit pci_device(phy_addr_t cfg_base, bdf_t bdf) : cfg_base_(cfg_base), bdf_(bdf) {}

    bdf_t bdf() const { return bdf_; }                ///< Returns the BDF.
    phy_addr_t cfg_base() const { return cfg_base_; } ///< Returns the config space start address.

    /// Returns the config space page as interval.
    cbl::interval cfg_page() const { return cbl::interval::from_size(static_cast<uintptr_t>(cfg_base()), PAGE_SIZE); }

    /// Indicates if the device is valid (first two bytes are not 0 and not 0xFFFF).
    bool is_valid() const { return vendor_id() != 0 and vendor_id() != math::mask(16); }

    uint16_t device_id() const { return read(offset::DEVICE_VENDOR_ID, shifts::DEVICE_ID); } ///< Gets Device ID.
    uint16_t vendor_id() const { return read(offset::DEVICE_VENDOR_ID, shifts::VENDOR_ID); } ///< Gets Vendor ID.
    uint8_t cap_offset() const { return read(offset::CAP_PTR); }                             ///< Gets cap ptr.
    uint8_t class_code() const { return read(offset::CLASS, shifts::CLASS); }                ///< Gets class.
    uint8_t subclass() const { return read(offset::CLASS, shifts::SUBCLASS); }               ///< Gets subclass.
    uint8_t prog_if() const { return read(offset::CLASS, shifts::PROG_IF); }                 ///< Gets PROG_IF.

    uint8_t type() const { return read(offset::HEADER_TYPE, shifts::DEV_TYPE) & masks::DEV_TYPE; } ///< Gets type.
    uint8_t bus_primary() const { return read(offset::BUS_INFO, shifts::BUS_PRIMARY); } ///< Gets primary bus number.
    uint8_t bus_secondary() const
    {
        return read(offset::BUS_INFO, shifts::BUS_SECONDARY);
    } ///< Gets secondary bus number.
    uint8_t bus_subordinate() const
    {
        return read(offset::BUS_INFO, shifts::BUS_SUBORDINATE);
    } ///< Gets subordinate bus number.

    /// Returns the number of BARs.
    unsigned bar_count() const { return type() == PCI_TYPE_GENERAL ? PCI_NUM_BARS : PCI_NUM_BARS_BRIDGE; }

    /// Indicates if this device is a PCI-to-PCI bridge.
    bool is_bridge() const { return type() == PCI_TYPE_PCI_BRIDGE; }

    /// Indicates if this device is an xHCI controller.
    bool is_xhci() const
    {
        return class_code() == PCI_CLASS_SERIAL and subclass() == PCI_SUBCLASS_USB and prog_if() == PCI_PROGIF_XHCI;
    }

    /// Indicates if this device is a PCI serial device.
    bool is_pci_serial() const { return class_code() == PCI_CLASS_SIMPLE_COMM and subclass() == PCI_SUBCLASS_SERIAL; }

    using bar_t = pci::bar_t; ///< Alias to the BAR structure.

    /// Returns a pointer to BAR i.
    bar_t* bar(unsigned i) const
    {
        return reinterpret_cast<bar_t*>(uintptr_t(cfg_base_) + uintptr_t(offset::BAR) + i * sizeof(uint32_t));
    }

    using capability = pci::capability;             ///< Alias to the capability structure.
    using ext_capability = pci::ext_capability;     ///< Alias to the capability structure.
    using msi_capability = pci::msi_capability;     ///< Alias to the MSI capability structure.
    using msix_capability = pci::msix_capability;   ///< Alias to the MSI-X capability structure.
    using sriov_capability = pci::sriov_capability; ///< Alias to the SR-IOV capability structure.

protected:
    class bar_iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = bar_t*;
        using difference_type = ptrdiff_t;
        using pointer = bar_t**;
        using reference = bar_t*&;

        static constexpr unsigned BAR_INDEX_INVALID {0xff};

        /// Constructs an iterator for the BARS on dev with initial index bar_index.
        bar_iterator(const pci_device& dev, unsigned bar_index) : dev_(dev), bar_index_(bar_index) {}

        /// Returns the BAR pointer for the current index.
        bar_t* operator*() const { return dev_.bar(bar_index_); }

        /// Indicates if the iterator o references a different BAR index.
        bool operator!=(const bar_iterator& o) const { return o.bar_index_ != bar_index_; }

        /// Jump to next BAR, honoring 64-bit BARs spanning two slots in the process.
        bar_iterator& operator++()
        {
            bar_index_ += operator*()->is_64bit() ? 2 : 1;
            if (bar_index_ >= dev_.bar_count()) {
                bar_index_ = BAR_INDEX_INVALID;
            }

            return *this;
        }

    private:
        const pci_device& dev_;
        unsigned bar_index_;
    };

    class capability_iterator
    {
        static constexpr uint8_t CAP_OFFSET_INVALID {0xff};

    public:
        /// Constructs an iterator for the device dev with initial offset cap_off.
        capability_iterator(const pci_device& dev, uint8_t cap_off) : dev_(dev), off(cap_off ?: CAP_OFFSET_INVALID) {}

        /// Returns the capability for the current offset.
        capability operator*() const { return *reinterpret_cast<capability*>(uintptr_t(dev_.cfg_base()) + off); }

        /// Indicates if the iterator o references a different capability.
        bool operator!=(const capability_iterator& o) const { return o.off != off; }

        /// Jump to next capability.
        capability_iterator& operator++()
        {
            off = operator*().next ?: CAP_OFFSET_INVALID;
            return *this;
        }

    private:
        const pci_device& dev_;
        uint8_t off;
    };

public:
    /// Helper class to provide an iterator over BARs.
    class bar_list
    {
    public:
        explicit bar_list(const pci_device& dev) : dev_(dev) {}

        bar_iterator begin() const { return {dev_, 0}; }
        bar_iterator end() const { return {dev_, bar_iterator::BAR_INDEX_INVALID}; }

    private:
        const pci_device& dev_;
    };

    bar_list get_bars() const { return bar_list(*this); }

    /// Helper class to provide capability iterator.
    class capability_list
    {
    public:
        /// Constructs a helper for the given PCI device dev.
        explicit capability_list(const pci_device& dev) : dev_(dev) {}

        capability_iterator begin() const { return {dev_, dev_.cap_offset()}; } ///< Begin iterator.
        capability_iterator end() const { return {dev_, 0}; }                   ///< End iterator.

    private:
        const pci_device& dev_;
    };

    /// Returns an iterable list of capabilities.
    capability_list get_capabilities() const { return capability_list(*this); }

    /// Returns the offset to the capability with the ID id, or 0, if not found.
    size_t find_cap(uint16_t id) const
    {
        uint8_t off {cap_offset()};
        while (off and off != 0xff) {
            auto* cap = reinterpret_cast<volatile capability*>(uintptr_t(cfg_base_) + off);
            if (cap->id == id) {
                return off;
            }
            off = cap->next;
        }
        return 0;
    }

    size_t find_ext_cap(uint16_t id) const
    {
        if (not find_cap(pci::pcie_capability::ID)) {
            // Non-PCIe devices do not have an extended config space.
            return 0;
        }

        uint16_t off {256}; // The extended config space always starts at the end of the legacy space.
        while (off and off != 0xfff) {
            auto* cap = reinterpret_cast<volatile ext_capability*>(uintptr_t(cfg_base_) + off);
            if (cap->id == id) {
                return off;
            }
            off = cap->version_and_next >> 4;
        }
        return 0;
    }

    size_t find_msi_cap() const { return find_cap(msi_capability::ID); }   ///< Finds the MSI capability offset.
    size_t find_msix_cap() const { return find_cap(msix_capability::ID); } ///< Finds the MSI-X capability offset.
    size_t find_sriov_cap() const
    {
        return find_ext_cap(sriov_capability::ID);
    } ///< Finds the SR-IOV capability offset.

    using msix_entry = pci::msix_entry; ///< Alias to the MSI-X entry structure.

private:
    template <typename T>
    volatile T* ptr(uintptr_t off) const
    {
        return reinterpret_cast<volatile T*>(uintptr_t(cfg_base_) + off);
    }

    uint32_t read(offset off) const { return *ptr<const uint32_t>(uintptr_t(off)); }
    void write(offset off, uint32_t value) { *ptr<uint32_t>(uintptr_t(off)) = value; }

    uint32_t read(offset off, shifts shift) const { return read(off) >> size_t(shift); }

    phy_addr_t cfg_base_;
    bdf_t bdf_;
};
