/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "x86/arch.hpp"
#include "memory/buddy.hpp"
#include "cbl/math.hpp"
#include "optional"

/* This pool always allocates 4k Bytes of memory.
 */

class page_pool
{
public:
    /* Allocs 4k Bytes of memory, aligned to 4k.
     * Panics if there is no free memory.
     */
    phy_addr_t alloc()
    {
        std::optional<uintptr_t> addr = bud.alloc(PAGE_BITS);
        PANIC_UNLESS(addr, "Page pool got no address, we are out of memory!");
        return phy_addr_t(*addr);
    }

    void free(phy_addr_t addr) { bud.free(uintptr_t(addr), PAGE_BITS); }

private:
    buddy bud {PAGE_BITS};
};
