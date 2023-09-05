/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <optional>
#include <page_table_base.hpp>

/* A base class for the pdpt and pd, as they have much in common.
 */
class paging_directory_entry_base : public paging_entry_base
{
protected:
    enum
    {
        D_SHIFT = 6,         // Dirty, ignored if pdpte maps a pdir
        PS_SHIFT = 7,        // Pagesize, must be 0 to map a pdir, 1 to map a 1gb page
        GL_SHIFT = 8,        // Global, ignored if pdpte maps a pdir
        PAT_SHIFT = 12,      // ignored if pdpte maps a pdir
        PROT_KEY_SHIFT = 59, // Protection key

        PROT_KEY_BITS = 4,

        D_MASK = math::mask(1, D_SHIFT),
        PS_MASK = math::mask(1, PS_SHIFT),
        GL_MASK = math::mask(1, GL_SHIFT),
        PAT_MASK = math::mask(1, PAT_SHIFT),
        PROT_KEY_MASK = math::mask(PROT_KEY_BITS, PROT_KEY_SHIFT),
    };

    paging_directory_entry_base() = default;
    using paging_entry_base::paging_entry_base;

    struct paging_directory_entry_t
    {
        uint64_t address {0};
        bool present {false};
        bool readwrite {false};
        bool usermode {false};
        bool pwt {false};
        bool pcd {false};
        bool accessed {false};
        bool dirty {false};
        bool pagesize {false};
        bool global {false};
        bool pat {false};
        uint8_t key {0};
        bool execute {false};
    };

public:
    std::optional<uint8_t> get_prot_key() const
    {
        if (is_large() && is_present()) {
            return {static_cast<uint8_t>((raw() & PROT_KEY_MASK) >> PROT_KEY_SHIFT)};
        }
        return {};
    }

    bool is_dirty() const { return is_large() and (raw() & D_MASK); }
    bool is_large() const { return is_present() and (raw() & PS_MASK); }
    bool is_global() const { return is_large() and (raw() & GL_MASK); }
    bool is_pat() const { return is_large() and (raw() & PAT_MASK); }

protected:
    bool set_bits_if_small_ps(uint64_t clr_mask, uint64_t set_mask)
    {
        if (not is_large()) {
            set_bits(clr_mask, set_mask);
            return true;
        }
        return false;
    }

    bool set_bits_if_big_ps(uint64_t clr_mask, uint64_t set_mask)
    {
        if (is_large()) {
            set_bits(clr_mask, set_mask);
            return true;
        }
        return false;
    }
};
