/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <xhci/capability.hpp>

struct __PACKED__ xhci_legsup_capability : xhci_capability
{
    enum
    {
        ID = 1,

        BIOS_OWNED = 1u << 0,
        OS_OWNED = 1u << 8,

        SMI_ENABLE = 1u << 0,
        SMI_HOST_ERROR = 1u << 4,
        SMI_OS_OWNER = 1u << 13,
        SMI_PCI_CMD_ENABLE = 1u << 14,
        SMI_BAR_ENABLE = 1u << 15,

        SMI_ENABLE_MASK = SMI_ENABLE | SMI_HOST_ERROR | SMI_OS_OWNER | SMI_PCI_CMD_ENABLE | SMI_BAR_ENABLE,

        SMI_OWNER_CHANGE = 1u << 29,
        SMI_PCI_CMD = 1u << 30,
        SMI_BAR = 1u << 31,

        SMI_ACK_MASK = SMI_OWNER_CHANGE | SMI_PCI_CMD | SMI_BAR,
    };

    bool bios_owned() const { return specific() & BIOS_OWNED; }
    void clear_bios() { specific(specific() & ~BIOS_OWNED); }

    bool os_owned() const { return specific() & OS_OWNED; }
    void set_os() { specific(specific() | OS_OWNED); }

    void disable_and_ack_smis()
    {
        control &= ~SMI_ENABLE_MASK;
        control |= SMI_ACK_MASK;
    }

    volatile uint32_t control;
};
