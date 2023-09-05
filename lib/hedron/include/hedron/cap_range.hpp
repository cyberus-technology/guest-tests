/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "cap_pool.hpp"
#include "crd.hpp"
#include "generic_cap_range.hpp"
#include "syscalls.hpp"
#include <hedron/cap_sel.hpp>

#include <math.hpp>

namespace hedron
{
namespace impl
{
/// A backend allocator for capability selectors.
struct cap_pool_allocator_policy
{
    static hedron::cap_sel allocate(size_t num) { return allocate_cap_range(math::order_envelope(num)).a; }

#ifdef HEDRON_RECLAMATION_BUG_IS_FIXED
    static void free(hedron::cap_sel sel, size_t num)
    {
        reclaim_cap_range(cbl::interval::from_size(sel, num));
    }
#else
    static void free(hedron::cap_sel /*sel*/, size_t /*num*/)
    {}
#endif
};

/// The default revoke policy for capability RAII wrappers: Just revoke everything immediately.
struct revoke_policy
{
    static void revoke(hedron::cap_sel /*sel*/, size_t num)
    {
        for (size_t offset {0}; offset < num; offset++) {
#ifdef HEDRON_RECLAMATION_BUG_IS_FIXED
            ::hedron::revoke(hedron::crd {hedron::crd::OBJ_CAP, ::hedron::rights::full(), sel + offset, 0}, true);
#endif
        }
    }
};

} // namespace impl

/// A RAII-wrapper around a capability range allocated with the default allocator.
template <size_t SIZE>
using cap_range = generic_cap_range<SIZE, impl::cap_pool_allocator_policy, impl::revoke_policy>;

/// A RAII-wrapper around a single capability allocated with the default allocator.
using cap = cap_range<1>;

} // namespace hedron
