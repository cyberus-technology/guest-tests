/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "paging_directory_entry_base.hpp"
#include "x86/arch.hpp"

class PDE : public paging_directory_entry_base
{
    enum
    {
        LPAGE_SHIFT = 21,  // 2M page
        LPAGE_BITS = 31,
        LPAGE_MASK = math::mask(LPAGE_BITS, LPAGE_SHIFT),
    };

 public:
    using pd_entry_t = paging_directory_entry_base::paging_directory_entry_t;

    PDE() = default;
    explicit PDE(uint64_t raw)
        : paging_directory_entry_base(raw) {}

    /* Those static functions will set some entries of the given config according to the function you call
     * and return a pde.
     */
    static PDE pde_to_pt(pd_entry_t config);
    static PDE pde_to_2mb_page(pd_entry_t config);

 private:
    explicit PDE(const pd_entry_t& config);

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

    std::optional<phy_addr_t> get_pt() const;
    bool set_pt(phy_addr_t addr, tlb_invalidation invl);

    std::optional<phy_addr_t> get_page() const;
    bool set_page(phy_addr_t addr, tlb_invalidation invl);

 private:
    void invalidate_entry(tlb_invalidation invl) const;
    void access_helper(bool value, uint64_t mask, tlb_invalidation invl);
};

static_assert(sizeof(PDE) == sizeof(uint64_t), "A PD Entry must have the size of an uint64_t");

using PD = paging_structure_container<PDE>;
