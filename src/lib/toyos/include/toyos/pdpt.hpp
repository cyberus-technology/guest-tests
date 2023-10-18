/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "paging_directory_entry_base.hpp"
#include <toyos/x86/arch.hpp>

class PDPTE : public paging_directory_entry_base
{
    enum
    {
        LPAGE_SHIFT = 30,  // 1G page
        LPAGE_BITS = 22,
        LPAGE_MASK = math::mask(LPAGE_BITS, LPAGE_SHIFT),
    };

 public:
    using pdpt_entry_t = paging_directory_entry_base::paging_directory_entry_t;

    PDPTE() = default;
    explicit PDPTE(uint64_t raw)
        : paging_directory_entry_base(raw) {}

    /* Those static functions will set some entries of the given config according to the function you call
     * and return a pdpte.
     */
    static PDPTE pdpte_to_pdir(pdpt_entry_t config);
    static PDPTE pdpte_to_1gb_page(pdpt_entry_t config);

 private:
    explicit PDPTE(const pdpt_entry_t& config);

 public:
    void set_present(bool pres, tlb_invalidation invl);
    void set_writeable(bool wr, tlb_invalidation invl);
    void set_usermode(bool mode, tlb_invalidation invl);
    void set_pwt(bool pwt, tlb_invalidation invl);
    void set_pcd(bool pcd, tlb_invalidation invl);
    void set_accessed(bool acc, tlb_invalidation invl);
    bool set_dirty(bool dirty, tlb_invalidation invl);
    bool set_global(bool global, tlb_invalidation invl);
    bool set_pat(bool pat, tlb_invalidation invl);
    void set_exec_disable(bool exec, tlb_invalidation invl);

    bool set_prot_key(uint8_t key, tlb_invalidation invl);

    std::optional<phy_addr_t> get_pdir() const;
    bool set_pdir(phy_addr_t addr, tlb_invalidation invl);

    std::optional<phy_addr_t> get_page() const;
    bool set_page(phy_addr_t addr, tlb_invalidation invl);

 private:
    void invalidate_entry(tlb_invalidation invl) const;
    void access_helper(bool value, uint64_t mask, tlb_invalidation invl);
};

static_assert(sizeof(PDPTE) == sizeof(uint64_t), "A PDPT Entry must have the size of an uint64_t");

using PDPT = paging_structure_container<PDPTE>;
