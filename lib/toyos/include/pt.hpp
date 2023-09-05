/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <arch.hpp>
#include <optional>
#include <page_table_base.hpp>

class PTE : public paging_entry_base
{
    enum
    {
        D_SHIFT = 6,         // Dirty
        PAT_SHIFT = 7,       // Page attribute table
        GL_SHIFT = 8,        // Global
        PROT_KEY_SHIFT = 59, // Protection Key

        PROT_KEY_BITS = 4,

        D_MASK = math::mask(1, D_SHIFT),
        PAT_MASK = math::mask(1, PAT_SHIFT),
        GL_MASK = math::mask(1, GL_SHIFT),
        PROT_KEY_MASK = math::mask(PROT_KEY_BITS, PROT_KEY_SHIFT),
    };

public:
    struct pt_entry_t
    {
        uint64_t address {0};
        bool present {false};
        bool readwrite {false};
        bool usermode {false};
        bool pwt {false};
        bool pcd {false};
        bool accessed {false};
        bool dirty {false};
        bool pat {false};
        bool global {false};
        uint8_t key {0};
        bool execute {false};
    };

    PTE() = default;
    explicit PTE(uint64_t raw) : paging_entry_base(raw) {}
    explicit PTE(const pt_entry_t& config);

    void set_present(bool pres, tlb_invalidation invl);
    void set_writeable(bool wr, tlb_invalidation invl);
    void set_usermode(bool mode, tlb_invalidation invl);
    void set_pwt(bool pwt, tlb_invalidation invl);
    void set_pcd(bool pcd, tlb_invalidation invl);
    void set_accessed(bool acc, tlb_invalidation invl);
    void set_exec_disable(bool exec, tlb_invalidation invl);

    bool is_dirty() const;
    void set_dirty(bool dirty, tlb_invalidation invl);

    bool is_pat() const;
    void set_pat(bool pat, tlb_invalidation invl);

    bool is_global() const;
    void set_global(bool global, tlb_invalidation invl);

    std::optional<phy_addr_t> get_phys_addr() const;
    void set_phys_addr(phy_addr_t addr, tlb_invalidation invl);

    std::optional<uint8_t> get_prot_key() const;
    void set_prot_key(uint8_t key, tlb_invalidation invl);

private:
    void invalidate_entry(tlb_invalidation invl) const;
    void access_helper(bool value, uint64_t mask, tlb_invalidation invl);
};

static_assert(sizeof(PTE) == sizeof(uint64_t), "A PT Entry must have the size of an uint64_t");

using PT = paging_structure_container<PTE>;
