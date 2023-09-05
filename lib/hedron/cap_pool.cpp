/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <buddy.hpp>
#include <hedron/cap_pool.hpp>
#include <hedron/cap_sel.hpp>
#include <mutex.hpp>
#include <trace.hpp>

static buddy* cap_pool {nullptr};
static mutex* cap_mtx {nullptr};

void initialize_cap_pool(uint64_t cap_base, uint64_t cap_size)
{
    static buddy cap_pool_ {31};
    cap_pool = &cap_pool_;

    buddy_reclaim_range(cbl::interval::from_size(cap_base, cap_size), *cap_pool);

    static mutex cap_mtx_;
    cap_mtx = &cap_mtx_;
}

cbl::interval allocate_cap_range(size_t ord)
{
    ASSERT(cap_pool, "no cap pool");

    auto alloc_fn {[&]() {
        auto begin {cap_pool->alloc(ord)};
        PANIC_UNLESS(begin, "out of caps");
        return cbl::interval::from_order(*begin, ord);
    }};

    mutex::optional_guard _(cap_mtx);

    return alloc_fn();
}

hedron::cap_sel allocate_cap()
{
    return allocate_cap_range(0).a;
}

void reclaim_cap_range(cbl::interval ival)
{
    ASSERT(cap_pool, "no cap pool");
    ASSERT(cap_mtx, "no cap mutex");

    mutex::guard _(*cap_mtx);
    buddy_reclaim_range(ival, *cap_pool);
}
