/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <toyos/xhci/config.hpp>
#include <toyos/xhci/device.hpp>

void xhci_device::power_cycle_ports()
{
    for (auto portsc : get_ports()) {
        portsc->poweroff();
        adapter_.delay(DELAY_POWER);
        portsc->poweron();
    }
}

void xhci_device::reset_ports()
{
    for (auto portsc : get_ports()) {
        portsc->reset();
    }
}

bool xhci_device::do_handover() const
{
    auto cap_ = find_cap<legsup_capability>();
    if (not cap_) {
        trace(TRACE_XHCI, "Could not find legacy support capability.");
        return true;
    }
    auto cap = *cap_;

    if (cap->os_owned()) {
        return true;
    }

    cap->clear_bios();
    cap->set_os();
    while (not cap->os_owned()) {
        adapter_.delay(DELAY_RELAX);
    }

    cap->disable_and_ack_smis();

    return true;
}
