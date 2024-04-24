// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "algorithm"
#include "optional"

#include "../x86/arch.hpp"
#include "toyos/util/cast_helpers.hpp"
#include "toyos/util/device_driver_adapter.hpp"
#include "toyos/util/math.hpp"
#include <compiler.hpp>

#include "capability_registers.hpp"
#include "legsup_capability.hpp"

#include "portsc.hpp"

/**
 * Generic xHCI device abstraction.
 *
 * This class handles all device interactions that
 * are not directly related to the debug capability:
 *
 *  - Generic parameter registers
 *  - xHCI Extended Capabilities
 *  - Port control
 *  - BIOS handover
 */
class xhci_device
{
    using generic_reg = xhci_capability_registers::generic;
    using hccparams1 = xhci_capability_registers::hccparams1;
    using hcsparams1 = xhci_capability_registers::hcsparams1;

 public:
    explicit xhci_device(device_driver_adapter& adapter, phy_addr_t mmio_base)
        : adapter_(adapter), mmio_base_(mmio_base)
    {}

    using capability = xhci_capability;
    using legsup_capability = xhci_legsup_capability;

    /**
     * Search the extended capability list for
     * a specific type.
     *
     * \tparam T The type of the capability struct.
     * \return An optional containing a pointer to the struct, if found.
     */
    template<typename T>
    std::optional<T*> find_cap() const
    {
        const auto& caps = get_capabilities();

        auto find_fn = [](const capability* candidate) { return candidate->id() == T::ID; };

        auto cap = std::find_if(caps.begin(), caps.end(), find_fn);
        if (cap != caps.end()) {
            return static_cast<T*>(*cap);
        }

        return {};
    }

 protected:
    using portsc_t = xhci_portsc_t;

    portsc_t* get_portsc(unsigned port) const
    {
        auto port_reg_offset = portsc_t::BASE_OFFSET + (port - 1) * portsc_t::NEXT_OFFSET;
        return num_to_ptr<portsc_t>(mmio_base_ + get_reg<generic_reg>()->cap_length() + port_reg_offset);
    }

    void power_cycle_ports();
    void reset_ports();

    bool do_handover() const;

    class port_iterator
    {
     public:
        port_iterator(const xhci_device& dev, uint8_t port)
            : dev_(dev), port_(port) {}

        portsc_t* operator*() const
        {
            return dev_.get_portsc(port_);
        }

        bool operator!=(const port_iterator& o) const
        {
            return o.port_ != port_;
        }

        port_iterator& operator++()
        {
            port_++;
            return *this;
        }

     private:
        const xhci_device& dev_;
        uint8_t port_;
    };

    class port_list
    {
     public:
        explicit port_list(const xhci_device& dev)
            : dev_(dev) {}
        port_iterator begin() const
        {
            return { dev_, 1u };
        }

        port_iterator end() const
        {
            return { dev_, uint8_t(dev_.get_hcsparams1()->max_ports() + 1u) };
        }

     private:
        const xhci_device& dev_;
    };

    port_list get_ports() const
    {
        return port_list(*this);
    }

    class capability_iterator
    {
     public:
        using iterator_category = std::input_iterator_tag;
        using value_type = capability*;
        using difference_type = void;
        using pointer = void;
        using reference = void;

        static constexpr uint16_t CAP_OFFSET_INVALID{ 0xffff };

        capability_iterator(phy_addr_t mmio_base, uint16_t cap_off)
            : mmio_base_(mmio_base), cap_off_(cap_off ?: CAP_OFFSET_INVALID)
        {}

        capability* operator*() const
        {
            return num_to_ptr<capability>(uintptr_t(mmio_base_) + cap_off_ * sizeof(uint32_t));
        }

        bool operator!=(const capability_iterator& o) const
        {
            return o.cap_off_ != cap_off_;
        }

        capability_iterator& operator++()
        {
            auto next = operator*()->next();
            cap_off_ = next ? cap_off_ + next : CAP_OFFSET_INVALID;
            return *this;
        }

     private:
        phy_addr_t mmio_base_;
        uint16_t cap_off_;
    };

    class capability_list
    {
     public:
        capability_list(const xhci_device& dev, phy_addr_t mmio_base)
            : dev_(dev), mmio_base_(mmio_base) {}
        capability_iterator begin() const
        {
            return { mmio_base_, dev_.get_hccparams1()->ext_cap_offset_dwords() };
        }

        capability_iterator end() const
        {
            return { mmio_base_, 0 };
        }

     private:
        const xhci_device& dev_;
        phy_addr_t mmio_base_;
    };

    capability_list get_capabilities() const
    {
        return capability_list(*this, mmio_base_);
    }

    template<typename T>
    T* get_reg() const
    {
        return num_to_ptr<T>(mmio_base_ + T::OFFSET);
    }

    hccparams1* get_hccparams1() const
    {
        return get_reg<hccparams1>();
    }
    hcsparams1* get_hcsparams1() const
    {
        return get_reg<hcsparams1>();
    }

    device_driver_adapter& adapter_;
    phy_addr_t mmio_base_;
};
