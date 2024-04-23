// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <toyos/util/cast_helpers.hpp>
#include <toyos/util/interval.hpp>

#include "acpi_tables.hpp"

#include <compiler.hpp>

#include <cstring>

static constexpr uintptr_t BDA_EBDA_SEGMENT_PTR{ 0x40E };
static constexpr size_t BDA_EBDA_SHIFT{ 4 };
static constexpr std::array<char, 8> RSDP_SIGNATURE{ 'R', 'S', 'D', ' ', 'P', 'T', 'R', ' ' };

static constexpr size_t RSD_PTR_ALIGN{ 16 };

static acpi_rsdp* find_rsdp(const cbl::interval& ival)
{
    for (uintptr_t rsdp_ptr = ival.a; rsdp_ptr < ival.b; rsdp_ptr += RSD_PTR_ALIGN) {
        // The signature "RSD PTR " needs to be on a 16-byte boundary.
        if (not memcmp(num_to_ptr<const void>(rsdp_ptr), "RSD PTR ", 8)) {
            return num_to_ptr<acpi_rsdp>(rsdp_ptr);
        }
    }

    return nullptr;
}

/*
 * Finds the MCFG table from the given RSDP. If RSDP is not provided, the
 * function tries to find the structure.
 *
 * It is valid that this function returns a null pointer, when the MCFG is not
 * found.
 */
static acpi_mcfg* find_mcfg(const acpi_rsdp* rsdp = nullptr)
{
    // Find RSDP structure in either
    // (1) The first KiB of the EBDA
    // (2) In 0xE0000 - 0xFFFFF
    if (not rsdp) {
        uint16_t ebda_segment;
        memcpy(&ebda_segment, reinterpret_cast<uint16_t*>(BDA_EBDA_SEGMENT_PTR), sizeof(uint16_t));

        uintptr_t ebda_ptr = ebda_segment << BDA_EBDA_SHIFT;

        auto ebda_ival = cbl::interval::from_size(ebda_ptr, 1024);
        auto end_low_mb_ival = cbl::interval(0xe0000, 0x100000);

        rsdp = find_rsdp(ebda_ival);
        if (not rsdp) {
            rsdp = find_rsdp(end_low_mb_ival);
        }

        if (not rsdp) {
            return nullptr;
        }
    }

    if (rsdp->signature != RSDP_SIGNATURE) {
        return nullptr;
    }

    // Is null when directly booted by Cloud Hypervisor.
    if (!rsdp->rsdt) {
        return nullptr;
    }

    acpi_rsdt* rsdt = num_to_ptr<acpi_rsdt>(rsdp->rsdt);

    for (size_t idx{ 0 }; idx < rsdt->number_of_entries(); idx++) {
        char* table_pointer = num_to_ptr<char>(rsdt->entry(idx));
        if (not memcmp(table_pointer, "MCFG", 4)) {
            return reinterpret_cast<acpi_mcfg*>(table_pointer);
        }
    }

    return nullptr;
}
