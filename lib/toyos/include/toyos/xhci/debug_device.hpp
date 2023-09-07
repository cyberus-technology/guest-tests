/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "toyos/util/lock_free_queue.hpp"

#include "config.hpp"
#include "debug_capability.hpp"
#include "debug_structs.hpp"
#include "device.hpp"

/**
 * xHCI debug device
 *
 * This class is responsible for configuring the debug device:
 *  - Info and configuration structs
 *  - DMA buffers
 *
 * Once set up, it provides functions for transferring data
 * to and from the debug host.
 */
class xhci_debug_device : public xhci_device
{
    using dma_buffer = device_driver_adapter::dma_buffer_t;

    std::u16string STR0_EN_US {u"\x09\x04"};

public:
    enum class power_cycle_method
    {
        NONE,
        POWERCYCLE,
    };

    using dbc_capability = xhci_dbc_capability;

    xhci_debug_device(const std::u16string& manuf, const std::u16string& product, const std::u16string& serial,
                      device_driver_adapter& adapter, phy_addr_t mmio_base, power_cycle_method power_method)
        : xhci_device(adapter, mmio_base)
        , manuf_(manuf)
        , product_(product)
        , serial_(serial)
        , debug_structs_(adapter.allocate_pages(1))
        , event_ring_buffer_(adapter.allocate_pages(EVENT_RING_PAGES))
        , out_ring_buffer_(adapter.allocate_pages(OUT_RING_PAGES))
        , in_ring_buffer_(adapter.allocate_pages(IN_RING_PAGES))
        , in_data_buffer_(adapter.allocate_pages(IN_BUF_PAGES))
        , out_data_buffer_(adapter.allocate_pages(OUT_BUF_PAGES))
        , poll_mtx_(adapter.get_mutex())
        , init_mtx_(adapter.get_mutex())
        , power_method_(power_method)
    {
        setup_info_context();
    }

    /**
     * Initialize the device.
     *
     * Note that currently this is blocking (no timeout) until a connection has been
     * detected. In the worst case, this could mean that you have to physically
     * (re-)connect the cable to make it work.
     *
     * This function is also called when the connection is lost, e.g., due
     * to a xHC reset by the guest.
     *
     * Resetting the ports can be requested for the first initialization
     * to help automatic discovery on most hosts.
     *
     * \param reinit Signal re-initialization after a reset.
     * \return True, if the initialization was successful. False, if the handover failed.
     */
    bool initialize(bool reinit = false);

    /**
     * Event handling loop.
     *
     * This function consumes all items in the event ring buffer
     * and updates the respective rings.
     *
     * \param reinit Detect a lost connection and re-initialize the DbC.
     * \return True, if the connection is alive, False, it if was lost.
     */
    bool handle_events(bool reinit = false);

    /**
     * Write a single byte to the output buffer.
     *
     * If the buffer is full afterwards, it is flushed.
     */
    void write_byte(unsigned char c);

    /**
     * Write a string.
     *
     * This flushes the current buffer, and then transfers the entire string,
     * using as many transfer requests as necessary.
     */
    void write_line(const std::string& str);

    /// Flush the output buffer to the bus.
    void flush();

private:
    uintptr_t get_info_context_addr() const { return debug_structs_.dma_address(info_ctx_); }

    void update_event_ring_dequeue_ptr();

    void setup_info_context();
    void setup_endpoint_contexts();
    void setup_event_ring();

    void queue_write_transfer(const std::string& str);

    dbc_capability* dbc_cap_ {*find_cap<dbc_capability>()};

    const std::u16string manuf_, product_, serial_;

    dma_buffer debug_structs_;
    dma_buffer event_ring_buffer_;
    dma_buffer out_ring_buffer_;
    dma_buffer in_ring_buffer_;

    dma_buffer in_data_buffer_;
    dma_buffer out_data_buffer_;

    xhci_debug_structs& get_debug_structs() const { return *static_cast<xhci_debug_structs*>(debug_structs_.lin_addr); }

    dbc_info_context& info_ctx_ {get_debug_structs().info_ctx};
    dbc_endpoint_context& out_ep_ctx_ {get_debug_structs().out_endpoint_ctx};
    dbc_endpoint_context& in_ep_ctx_ {get_debug_structs().in_endpoint_ctx};
    string_descriptors& info_strings_ {get_debug_structs().strings};
    event_ring_segment_table_entry& event_ring_segment_table {get_debug_structs().erst};

    using ev_buf_type = std::array<trb, EVENT_RING_SIZE>;
    using in_buf_type = std::array<trb, IN_RING_SIZE>;
    using out_buf_type = std::array<trb, OUT_RING_SIZE>;

    trb_ring<EVENT_RING_SIZE, false> event_ring_ {*static_cast<ev_buf_type*>(event_ring_buffer_.lin_addr),
                                                  event_ring_buffer_.dma_addr};

    trb_ring<OUT_RING_SIZE> out_ring_ {*static_cast<out_buf_type*>(out_ring_buffer_.lin_addr),
                                       out_ring_buffer_.dma_addr};
    trb_ring<IN_RING_SIZE> in_ring_ {*static_cast<in_buf_type*>(in_ring_buffer_.lin_addr), in_ring_buffer_.dma_addr};

    std::string output_buffer_;

    using mutex = device_driver_adapter::mutex_interface;
    std::unique_ptr<mutex> poll_mtx_;
    std::unique_ptr<mutex> init_mtx_;

    power_cycle_method power_method_;
};
