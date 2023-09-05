/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <mm.hpp>
#include <pdpt.hpp>

PDPTE PDPTE::pdpte_to_pdir(pdpt_entry_t config)
{
    config.dirty = false;
    config.pagesize = false;
    config.global = false;
    config.pat = false;
    config.key = 0;
    return PDPTE(config);
}

PDPTE PDPTE::pdpte_to_1gb_page(pdpt_entry_t config)
{
    config.pagesize = true;
    return PDPTE(config);
}

PDPTE::PDPTE(const pdpt_entry_t& config)
{
    uint64_t raw = 0;
    raw |= config.present * PR_MASK;
    raw |= config.readwrite * RW_MASK;
    raw |= config.usermode * US_MASK;
    raw |= config.pwt * PWT_MASK;
    raw |= config.pcd * PCD_MASK;
    raw |= config.accessed * A_MASK;
    raw |= config.dirty * D_MASK;
    raw |= config.pagesize * PS_MASK;
    raw |= config.global * GL_MASK;
    raw |= config.pat * PAT_MASK;
    raw |= config.execute * XD_MASK;

    if (config.pagesize) {
        raw |= config.address & LPAGE_MASK;
        raw |= (static_cast<uint64_t>(config.key) << PROT_KEY_SHIFT) & PROT_KEY_MASK;
    } else {
        raw |= config.address & ADDR_MASK;
    }

    raw_ = raw;
}

void PDPTE::set_present(bool pres, tlb_invalidation invl)
{
    access_helper(pres, PR_MASK, invl);
}

void PDPTE::set_writeable(bool wr, tlb_invalidation invl)
{
    access_helper(wr, RW_MASK, invl);
}

void PDPTE::set_usermode(bool mode, tlb_invalidation invl)
{
    access_helper(mode, US_MASK, invl);
}

void PDPTE::set_pwt(bool pwt, tlb_invalidation invl)
{
    access_helper(pwt, PWT_MASK, invl);
}

void PDPTE::set_pcd(bool pcd, tlb_invalidation invl)
{
    access_helper(pcd, PCD_MASK, invl);
}

void PDPTE::set_accessed(bool acc, tlb_invalidation invl)
{
    access_helper(acc, A_MASK, invl);
}

bool PDPTE::set_dirty(bool dirty, tlb_invalidation invl)
{
    if (set_bits_if_big_ps(D_MASK, dirty * D_MASK)) {
        invalidate_entry(invl);
        return true;
    }
    return false;
}

bool PDPTE::set_global(bool global, tlb_invalidation invl)
{
    if (set_bits_if_big_ps(GL_MASK, global * GL_MASK)) {
        invalidate_entry(invl);
        return true;
    }
    return false;
}

bool PDPTE::set_pat(bool pat, tlb_invalidation invl)
{
    if (set_bits_if_big_ps(PAT_MASK, pat * PAT_MASK)) {
        invalidate_entry(invl);
        return true;
    }
    return false;
}

void PDPTE::set_exec_disable(bool exec, tlb_invalidation invl)
{
    access_helper(exec, XD_MASK, invl);
}

bool PDPTE::set_prot_key(uint8_t key, tlb_invalidation invl)
{
    if (set_bits_if_big_ps(PROT_KEY_MASK, static_cast<uint64_t>(key) << PROT_KEY_SHIFT)) {
        invalidate_entry(invl);
        return true;
    }
    return false;
}

std::optional<phy_addr_t> PDPTE::get_pdir() const
{
    if (not is_large() && is_present()) {
        return {phy_addr_t(raw() & ADDR_MASK)};
    }
    return {};
}

bool PDPTE::set_pdir(phy_addr_t addr, tlb_invalidation invl)
{
    if (set_bits_if_small_ps(ADDR_MASK, uintptr_t(addr))) {
        invalidate_entry(invl);
        return true;
    }
    return false;
}

std::optional<phy_addr_t> PDPTE::get_page() const
{
    if (is_large() && is_present()) {
        return {phy_addr_t(raw() & LPAGE_MASK)};
    }
    return {};
}

bool PDPTE::set_page(phy_addr_t addr, tlb_invalidation invl)
{
    if (set_bits_if_big_ps(LPAGE_MASK, uintptr_t(addr))) {
        invalidate_entry(invl);
        return true;
    }
    return false;
}

void PDPTE::invalidate_entry(tlb_invalidation invl) const
{
    if (invl == tlb_invalidation::no) {
        return;
    }
    if (is_global()) {
        memory_manager::invalidate_tlb_all();
    } else if (is_large()) {
        memory_manager::invalidate_tlb(memory_manager::phy_to_lin(*get_page()));
    } else {
        memory_manager::invalidate_tlb_non_global();
    }
}

void PDPTE::access_helper(bool value, uint64_t mask, tlb_invalidation invl)
{
    set_bits(mask, value * mask);
    invalidate_entry(invl);
}
