/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <array>
#include <string>

#include <stdint.h>

#include "console.hpp"
#include "toyos/acpi_tables.hpp"
#include "toyos/pci/bus.hpp"
#include "toyos/util/algorithm.hpp"
#include "toyos/util/interval.hpp"
#include "toyos/x86/arch.hpp"
#include <config.hpp>

class console_serial_base : public console
{
 protected:
    uint16_t base{ 0 };

 public:
    struct console_info
    {
        uint16_t port{ SERIAL_PORT_DEFAULT };
        unsigned baud{ SERIAL_BAUD };
        unsigned irq{ SERIAL_IRQ_DEFAULT };

        explicit console_info(const std::string& arg)
        {
            const auto args{ tokenize(arg, ',') };

            switch (args.size()) {
                case 2:
                    irq = std::stoul(args[1]);
                    FALL_THROUGH;
                case 1:
                    port = std::stoul(args[0], nullptr, 16);
                    FALL_THROUGH;
                case 0:
                    break;
                    DEFAULT_TO_PANIC("Wrong number of arguments supplied for serial console.");
            }
        }
    };

    enum
    {
        THB = 0,
        DLL = 0,
        IER = 1,
        DLH = 1,
        FCR = 2,
        LCR = 3,
        MCR = 4,
        LSR = 5,
        MSR = 6,

        LCR_8BIT = 3u << 0,

        FCR_ENABLE = 1u << 0,
        FCR_CLEAR = 3u << 1,

        MCR_DTR = 1u << 0,
        MCR_RTS = 1u << 1,
        MCR_LOOP = 1u << 4,

        MSR_CTS = 1u << 4,
        MSR_DSR = 1u << 5,
        MSR_CARRIER = 1u << 7,

        THB_EMPTY = 1u << 5,
        DHR_EMPTY = 1u << 6,

        LSR_BOTH_EMPTY = THB_EMPTY | DHR_EMPTY,

        DLAB = 1u << 7,

        IIR = 2,

        IIR_NO_IRQ = 1u << 0,
        IIR_RECV = 2u << 1,
        IIR_SEND = 1u << 1,

        IIR_FIFO_ENABLED = 3u << 6,

        MCR_IRQ = 1u << 3,

        IER_RECV = 1u << 0,
        IER_SEND = 1u << 1,
        IER_STATUS = 3u << 2,

        IER_MASK = IER_RECV | IER_SEND | IER_STATUS,

        THB_DATA = 1u << 0,
    };

    console_serial_base(uint16_t port, unsigned);
    virtual ~console_serial_base() {}

    virtual void puts(const std::string& str) override
    {
        puts_default(str);
    }

    virtual void putc(char c) override;
};

class console_serial final : public console_serial_base
{
 public:
    console_serial(uint16_t port, unsigned baud)
        : console_serial_base(port, baud) {}

    virtual void putc(char c) override
    {
        console_serial_base::putc(c);
    }

    static void putchar(unsigned char c);
};

struct bda_serial_config
{
    enum
    {
        BDA_SERIAL_ADDR = 0x400,
    };

    static cbl::interval_impl<page_num_t> bda_pn_range()
    {
        return cbl::interval_impl<page_num_t>::from_size(page_num_t::from_address(BDA_SERIAL_ADDR), page_num_t{ 1 });
    }

    std::array<uint16_t, 4> ports;
};

/**
 * Finds the serial port in the BDA or returns the default COM1 port if it is
 * not present.
 * @return
 */
inline uint16_t find_serial_port_in_bda()
{
    bda_serial_config* bda{ reinterpret_cast<bda_serial_config*>(bda_serial_config::BDA_SERIAL_ADDR) };
    return find_if_or(
        bda->ports, [](auto p) { return p; }, SERIAL_PORT_DEFAULT);
}

/**
 * Applies port offset quirks for a list of known PCI serial cards and returns
 * the actual I/O port.
 */
inline uint16_t serial_port_offset_quirks(uint16_t iobase, uint16_t vendor, uint16_t device)
{
    // XR16850 (WCH382) PCIe 2 port serial card.
    if (vendor == 0x1c00 and device == 0x3253) {
        return iobase + 0xc0;
    }

    // No additional offset quirks required.
    return iobase;
}

/**
 * Try to discover a serial port.
 * We try to discover PCI Serial devices as first option, afterwards the BDA is
 * searched. If none of the above discovery mechanisms detects a serial port,
 * the default COM1 port is used.
*/
inline uint16_t discover_serial_port(acpi_mcfg* mcfg)
{
    if (not mcfg) {
        return find_serial_port_in_bda();
    }

    auto serial_port = find_serial_port_in_bda();

    pci_bus pcibus(static_cast<phy_addr_t>(mcfg->base), mcfg->busses());
    auto pci_serial_dev{
        std::find_if(pcibus.begin(), pcibus.end(), [](const pci_device& dev) { return dev.is_pci_serial(); })
    };

    if (pci_serial_dev == pcibus.end()) {
        return serial_port;
    }

    for (unsigned i{ 0 }; i < PCI_NUM_BARS; i++) {
        auto bar{ (*pci_serial_dev).bar(i) };
        if (not bar->is_pio()) {
            continue;
        }

        serial_port = serial_port_offset_quirks(
            bar->address(),
            (*pci_serial_dev).vendor_id(),
            (*pci_serial_dev).device_id());
        break;
    }

    return serial_port;
}

void serial_init(uint16_t port_begin);
