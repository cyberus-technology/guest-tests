// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "page_table_base.hpp"
#include "x86/arch.hpp"
#include <optional>

class PML4E : public paging_entry_base
{
 public:
    struct pml4_entry_t
    {
        uint64_t address{ 0 };
        bool present{ false };
        bool readwrite{ false };
        bool usermode{ false };
        bool pwt{ false };
        bool pcd{ false };
        bool accessed{ false };
        bool execute{ false };
    };

    PML4E() = default;
    explicit PML4E(uint64_t raw)
        : paging_entry_base(raw) {}
    explicit PML4E(const pml4_entry_t& config);

    void set_present(bool pres, tlb_invalidation invl);
    void set_writeable(bool wr, tlb_invalidation invl);
    void set_usermode(bool mode, tlb_invalidation invl);
    void set_pwt(bool pwt, tlb_invalidation invl);
    void set_pcd(bool pcd, tlb_invalidation invl);
    void set_accessed(bool acc, tlb_invalidation invl);
    void set_exec_disable(bool exec, tlb_invalidation invl);

    std::optional<phy_addr_t> get_pdpt() const;
    void set_pdpt(phy_addr_t addr, tlb_invalidation invl);

 private:
    void invalidate_entry(tlb_invalidation invl) const;
    void access_helper(bool value, uint64_t mask, tlb_invalidation invl);
};

static_assert(sizeof(PML4E) == sizeof(uint64_t), "A PML4 Entry must have the size of an uint64_t");

using PML4 = paging_structure_container<PML4E>;
