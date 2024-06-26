// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <toyos/mm.hpp>
#include <toyos/pd.hpp>
#include <toyos/pdpt.hpp>
#include <toyos/pml4.hpp>
#include <toyos/pt.hpp>

/* A RAII-Style class that will save the value and the reference of the given paging entry and restore its
 * value on destruction.
 */

template<class ENTRY>
class page_guard
{
 public:
    page_guard(ENTRY& paging_entry)
        : entry_ref(paging_entry), entry_val(static_cast<uint64_t>(paging_entry)) {}

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
