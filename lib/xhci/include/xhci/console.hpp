/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "debug_device.hpp"


#include <arch.hpp>
#include <cbl/interval.hpp>
#include <cbl/console.hpp>
#include <cbl/mutex.hpp>

#include <chrono>
#include <memory>
#include <optional>
#include <string>

class xhci_console_base : public console
{
public:
    /**
     * Construct an xHCI console.
     *
     * This initializes the debug device and provides the
     * I/O functionality.
     *
     * It can be configured using a custom identifier
     * that will be published using the USB device's
     * SerialNumber descriptor field.
     *
     * \param identifier  The value to be used in SerialNumber.
     * \param mmio_region The MMIO BAR containing the xHC register set.
     * \param dma_region  The DMA region to be used as allocation pool.
     */
    xhci_console_base(std::unique_ptr<device_driver_adapter> adapter, const std::u16string& identifier,
                      const cbl::interval& mmio_region, xhci_debug_device::power_cycle_method power_method)
        : adapter_(std::move(adapter))
        , dbc_dev_(MANUFACTURER_STR, DEVICE_STR, identifier, *adapter_, phy_addr_t(mmio_region.a), power_method)
    {
        dbc_dev_.initialize();
    }

    virtual void puts(const std::string& str) override;

    virtual void putc(char c) override;

    static void putchar(unsigned char c);

    /**
     * Do queue maintenance and re-connect if the connection was lost.
     *
     * This function needs to be called regularly. It returns the relative time when it needs to be called again (at the
     * latest).
     */
    std::chrono::milliseconds poll()
    {
        dbc_dev_.handle_events(true /* auto-reconnect */);
        return DELAY;
    }

protected:
    // Polling delay
    static inline constexpr std::chrono::milliseconds DELAY {20};

    std::unique_ptr<device_driver_adapter> adapter_;
    xhci_debug_device dbc_dev_;
};

void xhci_console_init(xhci_console_base& cons);
