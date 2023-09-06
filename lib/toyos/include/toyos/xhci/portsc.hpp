/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

struct xhci_portsc_t
{
    static constexpr size_t BASE_OFFSET {0x400};
    static constexpr size_t NEXT_OFFSET {0x10};
    enum
    {
        CONNECT_STATUS = 1u << 0,
        ENABLE_DISABLE = 1u << 1,
        RESET = 1u << 4,
        POWER = 1u << 9,
        CONNECT_CHANGE = 1u << 17,
        ENABLE_CHANGE = 1u << 18,
        RESET_CHANGE = 1u << 21,
    };

    bool is_connected() const { return raw & CONNECT_STATUS; }

    void reset() { raw |= RESET; }

    void poweroff() { raw &= ~POWER; }
    void poweron() { raw |= POWER; }

    volatile uint32_t raw;
};
