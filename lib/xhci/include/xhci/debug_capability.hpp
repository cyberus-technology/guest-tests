/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <math/math.hpp>

#include <xhci/device.hpp>

struct __PACKED__ xhci_dbc_capability : xhci_device::capability
{
    enum
    {
        ID = 10,

        DCERST_MAX_BITS = 5,
        DCERST_MAX_SHIFT = 16,

        MAX_BURST_SIZE_BITS = 8,
        MAX_BURST_SIZE_SHIFT = 16,

        DOORBELL_SHIFT = 8,

        CTRL_RUNNING = 1u << 0,
        CTRL_STATUS_EVENT = 1u << 1,
        CTRL_RUN_CHANGE = 1u << 4,
        CTRL_ENABLE = 1u << 31,

        STAT_ER = 1u << 0,

        PORTSC_CONNECT_STATUS_CHANGE = 1u << 17,
        PORTSC_PORT_RESET_CHANGE = 1u << 21,
        PORTSC_LINK_STATUS_CHANGE = 1u << 22,
        PORTSC_STATUS_EVENTS = PORTSC_CONNECT_STATUS_CHANGE | PORTSC_PORT_RESET_CHANGE | PORTSC_LINK_STATUS_CHANGE,

        PROTOCOL_CUSTOM = 0,
        PROTOCOL_GDB = 1,
    };

    bool is_running() const { return control & CTRL_RUNNING; }

    uint8_t dcerst_max() const { return (specific() >> DCERST_MAX_SHIFT) & math::mask(DCERST_MAX_BITS); }

    uint8_t max_burst_size() const { return (control >> MAX_BURST_SIZE_SHIFT) & math::mask(MAX_BURST_SIZE_BITS); }

    void enable() { control |= CTRL_ENABLE | CTRL_STATUS_EVENT; }
    void disable() { control &= ~(CTRL_ENABLE | CTRL_STATUS_EVENT); }

    bool run_change() const { return control & CTRL_RUN_CHANGE; }
    void clear_run_change() { control |= CTRL_RUN_CHANGE; }

    void clear_port_status_events() { port_status_control |= PORTSC_STATUS_EVENTS; }

    bool event_ring_not_empty() const { return status & STAT_ER; }

    void ring_doorbell_out() { doorbell = 0u << DOORBELL_SHIFT; }
    void ring_doorbell_in() { doorbell = 1u << DOORBELL_SHIFT; }

    // NOTE: If we find that xHCI controllers don't like byte/word accesses, we need to change this!
    volatile uint32_t doorbell;

    volatile uint16_t erst_size;
    volatile uint16_t reservedz2;

    volatile uint32_t reservedz3;

    volatile uint64_t erst_base;

    volatile uint64_t event_ring_dequeue_ptr;

    volatile uint32_t control;
    volatile uint32_t status;

    volatile uint32_t port_status_control;
    volatile uint32_t reservedp0;

    volatile uint64_t context_ptr;

    volatile uint8_t protocol;
    volatile uint8_t reservedz4;
    volatile uint16_t vendor_id;
    volatile uint16_t product_id;
    volatile uint16_t revision;
};
