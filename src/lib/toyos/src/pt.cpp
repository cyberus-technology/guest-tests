/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <toyos/mm.hpp>
#include <toyos/pt.hpp>

PTE::PTE(const pt_entry_t& config)
{
    uint64_t raw = 0;
    raw |= config.present * PR_MASK;
    raw |= config.readwrite * RW_MASK;
    raw |= config.usermode * US_MASK;
    raw |= config.pwt * PWT_MASK;
    raw |= config.pcd * PCD_MASK;
    raw |= config.accessed * A_MASK;
    raw |= config.dirty * D_MASK;
    raw |= config.pat * PAT_MASK;
    raw |= config.global * GL_MASK;
    raw |= config.execute * XD_MASK;
    raw |= config.address & ADDR_MASK;
    raw |= (static_cast<uint64_t>(config.key) << PROT_KEY_SHIFT) & PROT_KEY_MASK;
    raw_ = raw;
}

void PTE::set_present(bool pres, tlb_invalidation invl)
{
    access_helper(pres, PR_MASK, invl);
}

void PTE::set_writeable(bool wr, tlb_invalidation invl)
{
    access_helper(wr, RW_MASK, invl);
}

void PTE::set_usermode(bool mode, tlb_invalidation invl)
{
    access_helper(mode, US_MASK, invl);
}

void PTE::set_pwt(bool pwt, tlb_invalidation invl)
{
    access_helper(pwt, PWT_MASK, invl);
}

void PTE::set_pcd(bool pcd, tlb_invalidation invl)
{
    access_helper(pcd, PCD_MASK, invl);
}

void PTE::set_accessed(bool acc, tlb_invalidation invl)
{
    access_helper(acc, A_MASK, invl);
}

void PTE::set_exec_disable(bool exec, tlb_invalidation invl)
{
    access_helper(exec, XD_MASK, invl);
}

bool PTE::is_dirty() const
{
    return is_present() and (raw() & D_MASK);
}

void PTE::set_dirty(bool dirty, tlb_invalidation invl)
{
    access_helper(dirty, D_MASK, invl);
}

bool PTE::is_pat() const
{
    return is_present() and (raw() & PAT_MASK);
}

void PTE::set_pat(bool pat, tlb_invalidation invl)
{
    access_helper(pat, PAT_MASK, invl);
}

bool PTE::is_global() const
{
    return is_present() and (raw() & GL_MASK);
}

void PTE::set_global(bool global, tlb_invalidation invl)
{
    access_helper(global, GL_MASK, invl);
}

std::optional<phy_addr_t> PTE::get_phys_addr() const
{
    if (is_present()) {
        return { phy_addr_t(raw() & ADDR_MASK) };
    }
    return {};
}

void PTE::set_phys_addr(phy_addr_t addr, tlb_invalidation invl)
{
    set_bits(ADDR_MASK, uintptr_t(addr));
    invalidate_entry(invl);
}

std::optional<uint8_t> PTE::get_prot_key() const
{
    if (is_present()) {
        return { static_cast<uint8_t>((raw() & PROT_KEY_MASK) >> PROT_KEY_SHIFT) };
    }
    return {};
}

void PTE::set_prot_key(uint8_t key, tlb_invalidation invl)
{
    set_bits(PROT_KEY_MASK, static_cast<uint64_t>(key) << PROT_KEY_SHIFT);
    invalidate_entry(invl);
}

void PTE::invalidate_entry(tlb_invalidation invl) const
{
    if (invl == tlb_invalidation::no) {
        return;
    }
    if (is_global()) {
        memory_manager::invalidate_tlb_all();
    }
    else {
        memory_manager::invalidate_tlb(memory_manager::phy_to_lin(*get_phys_addr()));
    }
}

void PTE::access_helper(bool value, uint64_t mask, tlb_invalidation invl)
{
    set_bits(mask, value * mask);
    invalidate_entry(invl);
}
