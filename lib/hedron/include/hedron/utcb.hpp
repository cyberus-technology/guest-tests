/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

// Included first to avoid leaking dependencies.
#include <hedron/utcb_layout.hpp>

#include <optional>

#include <arch.hpp>
#include <cbl/traits.hpp>
#include <compiler.hpp>
#include <hedron/crd.hpp>
#include <hedron/mtd.hpp>
#include <hedron/typed_item.hpp>
#include <math.hpp>
#include <x86defs.hpp>

namespace hedron
{

class utcb
    : uncopyable
    , unmovable
    , public utcb_layout
{
public:
    uint64_t* begin_untyped() { return &items[0]; }
    uint64_t* end_untyped() { return &items[header.num_untyped]; }

    uint64_t* begin_typed() { return &items[NUM_ITEMS - header.num_typed * 2]; }
    uint64_t* end_typed() { return &items[NUM_ITEMS]; }

    uint64_t free_items() const { return NUM_ITEMS - header.num_untyped - header.num_typed * 2; }

    void add_typed_item(const typed_item& item)
    {
        ASSERT(free_items() >= 2, "Not enough space for a typed item");
        items[NUM_ITEMS - header.num_typed * 2 - 1] = item.get_flags();
        items[NUM_ITEMS - header.num_typed * 2 - 2] = item.get_value();

        header.num_typed++;
    }

    void add_untyped_item(uint64_t val)
    {
        ASSERT(free_items() >= 1, "Not enough space for an untyped item");
        items[header.num_untyped] = val;
        header.num_untyped++;
    }

    uint64_t get_untyped_item(size_t index) const
    {
        ASSERT(index < header.num_untyped, "invalid index: %lx", index);
        return items[index];
    }

    void clear_typed() { header.num_typed = 0; }
    void clear_untyped() { header.num_untyped = 0; }
    void clear()
    {
        clear_typed();
        clear_untyped();
    }

    bool can_inject() const
    {
        return not int_state.is_sti_blocking() and not int_state.is_mov_ss_blocking() and (rflags & x86::FLAGS_IF);
    }

    bool injection_inflight() const { return inj_info.valid(); }

    void set_injection_info(x86::intr_type type, uint8_t vec, std::optional<uint32_t> err = {})
    {
        ASSERT(not injection_inflight(), "an interrupt is already pending");

        inj_info.clear();
        inj_info.type(to_underlying(type));
        inj_info.vector(vec);
        if (err) {
            inj_info.error(true);
            inj_error = *err;
        }
        inj_info.valid(true);
    }

    void request_interrupt_window() { inj_info.request_interrupt_window(); }

    segment_descriptor& get_segment(x86::segment_register seg)
    {
        switch (seg) {
        case x86::segment_register::CS: return cs;
        case x86::segment_register::DS: return ds;
        case x86::segment_register::ES: return es;
        case x86::segment_register::FS: return fs;
        case x86::segment_register::GS: return gs;
        case x86::segment_register::SS: return ss;
        }
        __UNREACHED__;
    }

    /**
     * Dump the content of the UTCB on the terminal.
     */
    void dump() const;
};

static_assert(sizeof(utcb) == PAGE_SIZE, "UTCB must be exactly one page!");
static_assert(offsetof(utcb, cr4_mon) == 0x280, "UTCB ABI is violated");

static_assert(x86::NUM_PDPTE == 4, "Invalid number of PDPT entries");
static_assert(x86::PDPTE_SIZE == 8, "Invalid PDPTE size");

inline segment_descriptor to_descriptor(const x86::gdt_segment& rhs)
{
    return segment_descriptor {.sel = rhs.sel, .ar = rhs.ar, .limit = rhs.limit, .base = rhs.base};
}

} // namespace hedron
