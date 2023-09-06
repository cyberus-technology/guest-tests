/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include "assert.h"

#include <toyos/xhci/debug_device.hpp>

void xhci_debug_device::write_byte(unsigned char c)
{
    assert(output_buffer_.length() < OUT_BUF_SIZE);

    output_buffer_ += c;
    if (output_buffer_.length() == OUT_BUF_SIZE) {
        flush();
    }
}

void xhci_debug_device::write_line(const std::string& str)
{
    flush();

    static constexpr size_t CHUNK_SIZE {std::min(OUT_BUF_SIZE, TRANSFER_MAX)};

    for (size_t chunk {0}; chunk < (str.length() / CHUNK_SIZE) + 1; chunk++) {
        auto substring = str.substr(chunk * CHUNK_SIZE, CHUNK_SIZE);
        queue_write_transfer(substring);
    }
}

void xhci_debug_device::flush()
{
    if (output_buffer_.length() > 0) {
        queue_write_transfer(output_buffer_);
        output_buffer_ = "";
    }
}

void xhci_debug_device::update_event_ring_dequeue_ptr()
{
    dbc_cap_->event_ring_dequeue_ptr = event_ring_buffer_.dma_address(event_ring_.get_dequeue_ptr());
}

void xhci_debug_device::queue_write_transfer(const std::string& str)
{
    auto _ {init_mtx_->guard()};

    // Apparently, the debug device can't handle multiple outstanding requests.
    // Until we find out why this happens or how to improve it, we
    // block until the current request is done.
    while (not out_ring_.empty()) {
        // Help with the event loop to minimize latency
        if (not handle_events()) {
            // Drop request if connection was lost
            return;
        }
        dbc_cap_->ring_doorbell_out();
        adapter_.delay(DELAY_RELAX);
    }

    auto& trb = out_ring_.enqueue();
    trb.type(trb_normal::TYPE);
    auto index = out_ring_.index(ptr_to_num(&trb));

    trb.buffer = out_data_buffer_.dma_addr + index * OUT_BUF_SIZE;
    trb.length(str.length());
    str.copy(static_cast<char*>(out_data_buffer_.lin_addr) + index * OUT_BUF_SIZE, str.length());
    trb.set_ioc();
    trb.set_isp();
    trb.commit();

    dbc_cap_->ring_doorbell_out();
}
