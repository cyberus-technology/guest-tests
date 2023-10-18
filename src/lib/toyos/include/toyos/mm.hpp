/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "pd.hpp"
#include "pdpt.hpp"
#include "pml4.hpp"
#include "pt.hpp"
#include "toyos/util/math.hpp"
#include <toyos/x86/arch.hpp>

enum class paging_level
{
    PTE = 0,
    PDE = 1,
    PDPTE = 2,
};

class memory_manager
{
    enum
    {
        PML4_SHIFT = 12,
        PML4_OFF_SHIFT = 39,
        PDPT_OFF_SHIFT = 30,
        PD_OFF_SHIFT = 21,
        PT_OFF_SHIFT = 12,
        OFFSET_SHIFT = 0,

        PML4_BITS = 40,
        PML4_OFF_BITS = 9,
        PDPT_OFF_BITS = 9,
        PD_OFF_BITS = 9,
        PT_OFF_BITS = 9,
        OFFSET_BITS = 12,

        PML4_MASK = math::mask(PML4_BITS, PML4_SHIFT),
        PML4_OFF_MASK = math::mask(PML4_OFF_BITS, PML4_OFF_SHIFT),
        PDPT_OFF_MASK = math::mask(PDPT_OFF_BITS, PDPT_OFF_SHIFT),
        PD_OFF_MASK = math::mask(PD_OFF_BITS, PD_OFF_SHIFT),
        PT_OFF_MASK = math::mask(PT_OFF_BITS, PT_OFF_SHIFT),
    };

 public:
    // Write the address of the given pml4 into the cr3-register
    static void set_pml4(PML4& pml4);

    static void invalidate_tlb_non_global();
    static void invalidate_tlb_all();
    static void invalidate_tlb(lin_addr_t lin_addr);

    // The following 8 functions will return the paging structure of the paging entry
    // for the given address.
    static PML4& pml4();
    static PML4E& pml4_entry(lin_addr_t lin_addr);

    static PDPT& pdpt(lin_addr_t lin_addr);
    static PDPTE& pdpt_entry(lin_addr_t lin_addr);

    static PD& pd(lin_addr_t lin_addr);
    static PDE& pd_entry(lin_addr_t lin_addr);

    static PT& pt(lin_addr_t lin_addr);
    static PTE& pt_entry(lin_addr_t lin_addr);

    // Translate a phys address into a lin address and vice versa
    static phy_addr_t lin_to_phys(lin_addr_t lin_addr);
    static lin_addr_t phy_to_lin(phy_addr_t phy_addr);

 private:
    // The offset of a given address in the chosen paging structure
    static uint64_t pml4_offset(lin_addr_t lin_addr);
    static uint64_t pdpt_offset(lin_addr_t lin_addr);
    static uint64_t pd_offset(lin_addr_t lin_addr);
    static uint64_t pt_offset(lin_addr_t lin_addr);
    static uint64_t phys_offset(lin_addr_t lin_addr, paging_level level = paging_level::PTE);
};
