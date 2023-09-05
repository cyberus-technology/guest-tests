/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <mm.hpp>
#include <pd.hpp>
#include <pdpt.hpp>
#include <pml4.hpp>
#include <pt.hpp>

/* A RAII-Style class that will save the value and the reference of the given paging entry and restore its
 * value on destruction.
 */

template <class ENTRY>
class page_guard
{
public:
    page_guard(ENTRY& paging_entry) : entry_ref(paging_entry), entry_val(static_cast<uint64_t>(paging_entry)) {}

    ~page_guard()
    {
        entry_ref = ENTRY(entry_val);
        memory_manager::invalidate_tlb_non_global();
    }

private:
    ENTRY& entry_ref;
    uint64_t entry_val;
};

using pml4_guard = page_guard<PML4E>;
using pdpte_guard = page_guard<PDPTE>;
using pde_guard = page_guard<PDE>;
using pte_guard = page_guard<PTE>;
