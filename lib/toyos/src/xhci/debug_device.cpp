/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <cbl/cast_helpers.hpp>

#include <toyos/xhci/debug_device.hpp>

bool xhci_debug_device::initialize(bool reinit)
{
    auto _ {init_mtx_->guard()};

    if (not do_handover()) {
        return false;
    }

    dbc_cap_->disable();

    dbc_cap_->protocol = dbc_capability::PROTOCOL_CUSTOM;
    dbc_cap_->vendor_id = XHCI_VENDOR_ID;
    dbc_cap_->product_id = XHCI_PRODUCT_ID;
    dbc_cap_->revision = XHCI_REVISION;
    dbc_cap_->context_ptr = get_info_context_addr();

    setup_event_ring();

    if (not reinit) {
        setup_endpoint_contexts();
        dbc_cap_->enable();

        if (power_method_ == power_cycle_method::POWERCYCLE) {
            // Some debug hosts seem to need a little more time
            // before we powercycle the ports to trigger automatic
            // detection.
            adapter_.delay(DELAY_INIT);
            power_cycle_ports();
        }
    } else {
        dbc_cap_->clear_run_change();
        dbc_cap_->enable();
    }

    while (not dbc_cap_->is_running()) {
        adapter_.delay(DELAY_RELAX);
    }

    dbc_cap_->ring_doorbell_in();

    adapter_.delay(DELAY_INIT);
    return dbc_cap_->is_running();
}

void xhci_debug_device::setup_info_context()
{
    info_strings_.string0.set(STR0_EN_US);
    info_strings_.manufacturer.set(manuf_);
    info_strings_.product.set(product_);
    info_strings_.serial.set(serial_);

    info_ctx_ = dbc_info_context {};

    info_ctx_.string0 = debug_structs_.dma_address(info_strings_.string0);
    info_ctx_.manufacturer = debug_structs_.dma_address(info_strings_.manufacturer);
    info_ctx_.product = debug_structs_.dma_address(info_strings_.product);
    info_ctx_.serial = debug_structs_.dma_address(info_strings_.serial);

    info_ctx_.string0_length = info_strings_.string0.length;
    info_ctx_.manufacturer_length = info_strings_.manufacturer.length;
    info_ctx_.product_length = info_strings_.product.length;
    info_ctx_.serial_length = info_strings_.serial.length;
}

void xhci_debug_device::setup_endpoint_contexts()
{
    out_ring_.initialize();

    out_ep_ctx_ = dbc_endpoint_context {};

    auto max_burst = dbc_cap_->max_burst_size();

    out_ep_ctx_.type(dbc_endpoint_context::BULK_OUT);
    out_ep_ctx_.set_dequeue_ptr(out_ring_buffer_.dma_address(out_ring_.get_dequeue_ptr()));

    out_ep_ctx_.average_trb = PAGE_SIZE;
    out_ep_ctx_.max_packet_size = MAX_PACKET_SIZE;
    out_ep_ctx_.max_burst_size = max_burst;

    in_ring_.initialize();

    in_ep_ctx_ = dbc_endpoint_context {};

    in_ep_ctx_.set_dequeue_ptr(in_ring_buffer_.dma_address(in_ring_.get_dequeue_ptr()));
    in_ep_ctx_.type(dbc_endpoint_context::BULK_IN);

    in_ep_ctx_.average_trb = MAX_PACKET_SIZE;
    in_ep_ctx_.max_packet_size = MAX_PACKET_SIZE;
    in_ep_ctx_.max_burst_size = max_burst;
}

void xhci_debug_device::setup_event_ring()
{
    event_ring_.initialize();

    dbc_cap_->erst_size = 1;
    dbc_cap_->erst_base = debug_structs_.dma_address(event_ring_segment_table);

    event_ring_.initialize();
    event_ring_segment_table.base_address = event_ring_buffer_.dma_addr;
    event_ring_segment_table.size = EVENT_RING_SIZE;

    update_event_ring_dequeue_ptr();
}

bool xhci_debug_device::handle_events(bool reinit)
{
    auto _ {poll_mtx_->guard()};

    if (not dbc_cap_->is_running()) {
        if (reinit) {
            initialize(true);
        } else {
            return false;
        }
    }

    event_ring_.update_enqueue_ptr();

    while (not event_ring_.empty()) {
        auto& trb = event_ring_.dequeue();
        switch (trb.type()) {
        case trb_port_status_change_event::TYPE: dbc_cap_->clear_port_status_events(); break;

        case trb_transfer_event::TYPE:
            if (trb.endpoint_id() == trb_transfer_event::ENDPOINT_ID_IN) {
                // We ignore the data that came with input events.
                in_ring_.update_dequeue_ptr(trb.buffer);
            } else {
                out_ring_.update_dequeue_ptr(trb.buffer);
            }
            break;
        default: info("Unknown event: Type {#x}", trb.type()); break;
        }
    }

    update_event_ring_dequeue_ptr();

    return true;
}
