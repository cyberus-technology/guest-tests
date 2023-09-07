/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <toyos/util/cast_helpers.hpp>
#include <toyos/irq_guard.hpp>
#include <toyos/mm.hpp>

using x86::cr4;

void memory_manager::set_pml4(PML4& pml4)
{
    uint64_t cr3_val = get_cr3();
    cr3_val = cr3_val & ~PML4_MASK;
    cr3_val = cr3_val | (reinterpret_cast<uintptr_t>(&pml4) & PML4_MASK);
    set_cr3(cr3_val);
}

void memory_manager::invalidate_tlb_non_global()
{
    set_cr3(get_cr3());
}

void memory_manager::invalidate_tlb_all()
{
    irq_guard guard;
    set_cr4(get_cr4() & ~math::mask_from(cr4::PGE));
    set_cr4(get_cr4() | math::mask_from(cr4::PGE));
}

void memory_manager::invalidate_tlb(lin_addr_t lin_addr)
{
    invlpg(uintptr_t(lin_addr));
}

PML4& memory_manager::pml4()
{
    phy_addr_t addr = phy_addr_t(get_cr3() & PML4_MASK);
    auto* p = num_to_ptr<PML4>(uintptr_t(memory_manager::phy_to_lin(addr)));
    return *p;
}

PML4E& memory_manager::pml4_entry(lin_addr_t lin_addr)
{
    uint64_t index = pml4_offset(lin_addr);
    return pml4()[index];
}

PDPT& memory_manager::pdpt(lin_addr_t lin_addr)
{
    phy_addr_t addr = *pml4_entry(lin_addr).get_pdpt();
    auto* p = num_to_ptr<PDPT>(uintptr_t(memory_manager::phy_to_lin(addr)));
    return *p;
}

PDPTE& memory_manager::pdpt_entry(lin_addr_t lin_addr)
{
    uint64_t index = pdpt_offset(lin_addr);
    return pdpt(lin_addr)[index];
}

PD& memory_manager::pd(lin_addr_t lin_addr)
{
    PDPTE pdpte = pdpt_entry(lin_addr);
    PANIC_ON(pdpte.is_large(), "Tried to get a PD, but PDPTE references 1GB page.");
    phy_addr_t pd_addr = *pdpte.get_pdir();
    auto* pd = num_to_ptr<PD>(uintptr_t(memory_manager::phy_to_lin(pd_addr)));
    return *pd;
}

PDE& memory_manager::pd_entry(lin_addr_t lin_addr)
{
    uint64_t index = pd_offset(lin_addr);
    return pd(lin_addr)[index];
}

PT& memory_manager::pt(lin_addr_t lin_addr)
{
    PDE pde = pd_entry(lin_addr);
    PANIC_ON(pde.is_large(), "Tried to get a PT, but PDE references 2MB page.");
    phy_addr_t pt_addr = *pde.get_pt();
    auto* pt = num_to_ptr<PT>(uintptr_t(memory_manager::phy_to_lin(pt_addr)));
    return *pt;
}

PTE& memory_manager::pt_entry(lin_addr_t lin_addr)
{
    uint64_t index = pt_offset(lin_addr);
    return pt(lin_addr)[index];
}

phy_addr_t memory_manager::lin_to_phys(lin_addr_t lin_addr)
{
    auto pdpte = pdpt_entry(lin_addr);
    if (pdpte.is_large()) {
        return phy_addr_t(uintptr_t(*pdpte.get_page()) | phys_offset(lin_addr, paging_level::PDPTE));
    }
    auto pde = pd_entry(lin_addr);
    if (pde.is_large()) {
        return phy_addr_t(uintptr_t(*pde.get_page()) | phys_offset(lin_addr, paging_level::PDE));
    }
    auto pte = pt_entry(lin_addr);
    return phy_addr_t(uintptr_t(*pte.get_phys_addr()) | phys_offset(lin_addr, paging_level::PTE));
}

lin_addr_t memory_manager::phy_to_lin(phy_addr_t phy_addr)
{
    return lin_addr_t(uintptr_t(phy_addr));
}

uint64_t memory_manager::pml4_offset(lin_addr_t lin_addr)
{
    uint64_t addr = uintptr_t(lin_addr);
    return (addr & PML4_OFF_MASK) >> PML4_OFF_SHIFT;
}

uint64_t memory_manager::pdpt_offset(lin_addr_t lin_addr)
{
    uint64_t addr = uintptr_t(lin_addr);
    return (addr & PDPT_OFF_MASK) >> PDPT_OFF_SHIFT;
}

uint64_t memory_manager::pd_offset(lin_addr_t lin_addr)
{
    uint64_t addr = uintptr_t(lin_addr);
    return (addr & PD_OFF_MASK) >> PD_OFF_SHIFT;
}

uint64_t memory_manager::pt_offset(lin_addr_t lin_addr)
{
    uint64_t addr = uintptr_t(lin_addr);
    return (addr & PT_OFF_MASK) >> PT_OFF_SHIFT;
}

uint64_t memory_manager::phys_offset(lin_addr_t lin_addr, paging_level level)
{
    uint64_t addr = uintptr_t(lin_addr);
    uint64_t bits = 0;

    switch (level) {
    case paging_level::PDPTE: bits += PD_OFF_BITS; FALL_THROUGH;
    case paging_level::PDE: bits += PT_OFF_BITS; FALL_THROUGH;
    case paging_level::PTE: FALL_THROUGH;
    default: bits += OFFSET_BITS;
    }

    uint64_t offset_mask = math::mask(bits, OFFSET_SHIFT);
    return (addr & offset_mask) >> OFFSET_SHIFT;
}
