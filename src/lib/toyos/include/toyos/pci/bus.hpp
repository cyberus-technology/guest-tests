/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "device.hpp"
#include "toyos/pci/pci.hpp"

#include <toyos/x86/arch.hpp>

/**
 * PCI Bus abstraction.
 *
 * This class can be used to enumerate PCI devices
 * present in the system.
 */
class pci_bus
{
    using bdf_t = pci_device::bdf_t;

    class iterator
    {
     public:
        using iterator_category = std::input_iterator_tag;
        using value_type = pci_device;
        using difference_type = void;
        using pointer = void;
        using reference = void;

        iterator(phy_addr_t cfg_base, phy_addr_t cfg_end)
            : cfg_base_(cfg_base), cfg_end_(cfg_end) {}

        pci_device operator*() const
        {
            return pci_device(cfg_base_, bdf_);
        }
        bool operator!=(const iterator& o) const
        {
            return o.cfg_base_ != cfg_base_;
        }
        bool operator==(const iterator& o) const
        {
            return o.cfg_base_ == cfg_base_;
        }

        iterator& operator++()
        {
            do {
                cfg_base_ += PAGE_SIZE;
                ++bdf_;
            } while (cfg_base_ != cfg_end_ and not pci_device(cfg_base_, bdf_).is_valid());
            return *this;
        }

     private:
        phy_addr_t cfg_base_;
        phy_addr_t cfg_end_;
        bdf_t bdf_{ 0, 0, 0 };
    };

 public:
    static constexpr const size_t BITS_PER_BUS{ 8 };  ///< Number of bits in a bus number.

    /**
     * Constructs an instance for access using the MMCFG method.
     *
     * The parameters for this can be found in the ACPI
     * MCFG table.
     *
     * Note that this is currently the only supported method.
     *
     * \param mcfg_base Base address for the configuration space.
     * \param busses    The number of busses to iterate over.
     */
    pci_bus(phy_addr_t mcfg_base, size_t busses)
        : mcfg_base_(mcfg_base), pci_busses_(busses) {}

    /// Returns begin iterator.
    iterator begin() const
    {
        return iterator(mcfg_base_, mcfg_end());
    }

    /// Returns end iterator.
    iterator end() const
    {
        return iterator(mcfg_end(), mcfg_end());
    }

 protected:
    /**
     * Translates an I/O port access (with address/data) into
     * an MMCFG address.
     *
     * \param rid    The requester ID identifying the PCI device.
     * \param offset The offset configured using the address register.
     * \param port   The data I/O port that is being accessed (indicating which and how many bytes to access).
     * \return The physical address corresponding to the desired access.
     */
    phy_addr_t io_to_mmio(uint16_t rid, uint8_t offset, uint16_t port) const
    {
        return phy_addr_t(uintptr_t(mcfg_base_) + (rid << PAGE_BITS) + offset + port - PCI_PORT_DATA);
    }

    /// Calculates the end of the MMCFG interval from the number of busses.
    phy_addr_t mcfg_end() const
    {
        return phy_addr_t(uintptr_t(mcfg_base_)
                          + pci_busses_ * MAX_DEVICES_PER_BUS * MAX_FUNCTIONS_PER_DEV * PAGE_SIZE);
    }

    phy_addr_t mcfg_base_;
    size_t pci_busses_;
};
