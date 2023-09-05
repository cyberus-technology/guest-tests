/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <mm.hpp>
#include <pml4.hpp>

PML4E::PML4E(const pml4_entry_t& config)
{
    uint64_t raw = 0;
    raw |= config.present * PR_MASK;
    raw |= config.readwrite * RW_MASK;
    raw |= config.usermode * US_MASK;
    raw |= config.pwt * PWT_MASK;
    raw |= config.pcd * PCD_MASK;
    raw |= config.accessed * A_MASK;
    raw |= config.execute * XD_MASK;
    raw |= config.address & ADDR_MASK;
    raw_ = raw;
}

void PML4E::set_present(bool pres, tlb_invalidation invl)
{
    access_helper(pres, PR_MASK, invl);
}

void PML4E::set_writeable(bool wr, tlb_invalidation invl)
{
    access_helper(wr, RW_MASK, invl);
}

void PML4E::set_usermode(bool mode, tlb_invalidation invl)
{
    access_helper(mode, US_MASK, invl);
}

void PML4E::set_pwt(bool pwt, tlb_invalidation invl)
{
    access_helper(pwt, PWT_MASK, invl);
}

void PML4E::set_pcd(bool pcd, tlb_invalidation invl)
{
    access_helper(pcd, PCD_MASK, invl);
}

void PML4E::set_accessed(bool acc, tlb_invalidation invl)
{
    access_helper(acc, A_MASK, invl);
}

void PML4E::set_exec_disable(bool exec, tlb_invalidation invl)
{
    access_helper(exec, XD_MASK, invl);
}

std::optional<phy_addr_t> PML4E::get_pdpt() const
{
    if (is_present()) {
        return {phy_addr_t(raw() & ADDR_MASK)};
    }
    return {};
}

void PML4E::set_pdpt(phy_addr_t addr, tlb_invalidation invl)
{
    set_bits(ADDR_MASK, uintptr_t(addr));
    invalidate_entry(invl);
}

void PML4E::invalidate_entry(tlb_invalidation invl) const
{
    if (invl == tlb_invalidation::no) {
        return;
    }
    memory_manager::invalidate_tlb_non_global();
}

void PML4E::access_helper(bool value, uint64_t mask, tlb_invalidation invl)
{
    set_bits(mask, value * mask);
    invalidate_entry(invl);
}
