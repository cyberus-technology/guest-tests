// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "../baremetal_device_driver_adapter.hpp"
#include "../xhci/console.hpp"

class xhci_console_baremetal final : public xhci_console_base
{
 public:
    xhci_console_baremetal(const std::u16string& identifier, const cbl::interval& mmio_region, const cbl::interval& dma_region, xhci_debug_device::power_cycle_method power_method)
        : xhci_console_base(std::make_unique<baremetal_device_driver_adapter>(dma_region), identifier, mmio_region, power_method)
    {}

    virtual void putc(char c) override
    {
        xhci_console_base::putc(c);
    }
};
