// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "chrono"

#include "../x86/arch.hpp"
#include "toyos/util/math.hpp"

#include "trb.hpp"

static const char16_t* const MANUFACTURER_STR{ u"Cyberus Technology GmbH" };
static const char16_t* const DEVICE_STR{ u"xHCI Console" };

// These values determine which driver on the debug host will attach to the debug device.
// For unknown IDs, the driver needs to be forced to feel responsible.
static constexpr uint16_t XHCI_VENDOR_ID{ 0xffff };
static constexpr uint16_t XHCI_PRODUCT_ID{ 0x0001 };
static constexpr uint16_t XHCI_REVISION{ 1 };

// Relaxing delay used while waiting for a condition
static constexpr std::chrono::microseconds DELAY_RELAX{ 50 };

// Power cycling delay (mentioned by the spec)
static constexpr std::chrono::milliseconds DELAY_POWER{ 20 };

// Delay before considering the console usable. This is a bit of guesswork,
// but there have been issues if this delay was too short or not present.
static constexpr std::chrono::seconds DELAY_INIT{ 1 };

// Endpoint context structure size. Mandated by the specification, do not change!
static constexpr size_t CONTEXT_SIZE{ 64 };

// This is a USB-defined constant. Do not change!
static constexpr size_t MAX_PACKET_SIZE{ 1024 };

// Each transfer request can hold up to this many bytes.
// For input, we don't expect large requests, so 1 KiB should be enough.
static constexpr size_t IN_BUF_SIZE{ MAX_PACKET_SIZE };
// For output, we want to transfer as much as possible.
static constexpr size_t OUT_BUF_SIZE{ 64 * 1024 };
// Note that the maximum is actually 64 KiB - 1, because the length
// field is only 16 bits.
static constexpr size_t TRANSFER_MAX{ 64 * 1024 - 1 };

// Requests (or events) are managed in ring buffers with this many entries.
static constexpr size_t EVENT_RING_SIZE{ 16 };
static constexpr size_t IN_RING_SIZE{ 16 };
// Currently, scheduling multiple transfers at a time hangs
// the controller. We don't need a large ring because
// we always wait for the previous transfer to be completed.
static constexpr size_t OUT_RING_SIZE{ 4 };

// All rings and transfer buffers need to be accessible via DMA.
// These constants provide shortcuts to size calculations when
// allocating these buffers.
static constexpr size_t EVENT_RING_PAGES{ size_to_pages(EVENT_RING_SIZE * sizeof(trb)) };
static constexpr size_t IN_RING_PAGES{ size_to_pages(IN_RING_SIZE * sizeof(trb)) };
static constexpr size_t OUT_RING_PAGES{ size_to_pages(OUT_RING_SIZE * sizeof(trb)) };

static constexpr size_t IN_BUF_PAGES{ size_to_pages(IN_BUF_SIZE * IN_RING_SIZE) };
static constexpr size_t OUT_BUF_PAGES{ size_to_pages(OUT_BUF_SIZE * OUT_RING_SIZE) };

static constexpr size_t XHCI_DMA_BUFFER_PAGES{ 1ul + EVENT_RING_PAGES + OUT_RING_PAGES + IN_RING_PAGES + IN_BUF_PAGES
                                               + OUT_BUF_PAGES };

static_assert(EVENT_RING_PAGES * PAGE_SIZE >= EVENT_RING_SIZE * sizeof(trb), "Not enough space for event ring");
static_assert(IN_RING_PAGES * PAGE_SIZE >= IN_RING_SIZE * sizeof(trb), "Not enough space for in ring");
static_assert(OUT_RING_PAGES * PAGE_SIZE >= OUT_RING_SIZE * sizeof(trb), "Not enough space for out ring");

static_assert(IN_BUF_PAGES * PAGE_SIZE >= IN_BUF_SIZE * IN_RING_SIZE, "Not enough space for input buffer");
static_assert(OUT_BUF_PAGES * PAGE_SIZE >= OUT_BUF_SIZE * OUT_RING_SIZE, "Not enough space for output buffer");
