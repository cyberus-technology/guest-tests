// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "../x86/arch.hpp"
#include <compiler.hpp>

#include "debug_contexts.hpp"
#include "ring.hpp"

struct __PACKED__ xhci_debug_structs
{
    // The order and alignment three contexts are mandated
    // by the spec (xHCI specification, 7.6.9).
    dbc_info_context info_ctx;
    dbc_endpoint_context out_endpoint_ctx;
    dbc_endpoint_context in_endpoint_ctx;

    // Locate the string descriptors in the same page to
    // save allocations.
    string_descriptors strings;

    // Locate the event ring segment table in the same page,
    // again to save allocations.
    event_ring_segment_table_entry erst;
};

static_assert(sizeof(xhci_debug_structs) < PAGE_SIZE, "Structure must not span page boundaries!");
