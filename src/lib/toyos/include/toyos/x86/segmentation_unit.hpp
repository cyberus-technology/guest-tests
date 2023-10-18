/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <toyos/x86/arch.hpp>
#include <toyos/x86/x86defs.hpp>

#include <optional>

class segmentation_unit
{
 public:
    struct segment_descriptor
    {
        x86::segment_register seg;
        uint64_t base;
        uint32_t limit;
        uint16_t ar;
    };

    segmentation_unit(const segment_descriptor& s, x86::cpu_mode mode)
        : segment_(s), mode_(mode) {}

    /**
     * Translate a logical address via this segment.
     * \param offset The offset part of the logical address
     * \param type   The type of the access (READ/WRITE/EXECUTE)
     * \param size   The number of bytes accessed
     * \return An optional containing a valid linear address if there was no exception during the translation.
     */
    std::optional<uintptr_t> translate_logical_address(uintptr_t offset, x86::memory_access_type_t, size_t)
    {
        // TODO: Add limit checks and handle special segments
        switch (mode_) {
            case x86::cpu_mode::PM64:
                return (segment_.seg == x86::segment_register::FS or segment_.seg == x86::segment_register::GS) ? segment_.base + offset : offset;
            default:
                return segment_.base + offset;
        };
    }

 private:
    const segment_descriptor segment_;
    const x86::cpu_mode mode_;
};
