/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <arch.h>
#include "buddy.hpp"
#include <cbl/cast_helpers.hpp>
#include "cbl/math.hpp"

#include <algorithm>
#include <functional>
#include <map>

/**
 * A simpler buddy allocator derived from the above buddy, allowing for allocation and deallocation without
 * knowledge of the order of the range of the claimed/freed memory. Furthermore this buddy claims memory
 * via a passed allocation function when running out of memory.
 */
template <class block_manager = heap_block_manager>
class simple_buddy_impl : public buddy_impl<block_manager>
{
    std::function<void*(size_t)> alloc_func;
    std::map<uintptr_t, math::order_t> alloc_map;
    static constexpr uint8_t min_claim_order {static_cast<uint8_t>(PAGE_BITS)};
    static constexpr uint8_t max_claim_order {static_cast<uint8_t>(63u)};

    using buddy_t = buddy_impl<block_manager>;

public:
    /**
     * Constructs a simple buddy.
     *
     * \param alloc_func The function to use to claim memory when out of memory.
     */
    explicit simple_buddy_impl(
        math::order_t max_ord,
        const std::function<void*(size_t)>& alloc_func_ = [](size_t s) { return operator new(s); })
        : buddy_t(max_ord), alloc_func(alloc_func_)
    {
        ASSERT(alloc_func != nullptr, "BUG: alloc function is null");
    }

    /**
     * Requests an address range of the given order. If the simple_buddy runs out of memory, it tries to claim
     * additional memory via the alloc_func.
     * The semantics of the allocation are the same as for buddy::alloc()
     *
     * \param order The order of the address range to allocate.
     * \return Naturally aligned optional start address of the range if successful, empty optional otherwise.
     */
    std::optional<uintptr_t> alloc(math::order_t order)
    {
        auto opt_addr {buddy_t::alloc(order)};
        if (not opt_addr) {
            // get fresh memory from heap
            math::order_t claim_order {std::clamp(static_cast<math::order_t>(order + 1u),
                                                  static_cast<math::order_t>(min_claim_order + 1u), max_claim_order)};

            auto new_space {alloc_func(static_cast<size_t>(1) << claim_order)};

            if (new_space == nullptr) {
                return {}; // Could not claim memory from default heap
            }

            buddy_reclaim_range(cbl::interval::from_order(ptr_to_num(new_space), claim_order), *this);

            opt_addr = buddy_t::alloc(order);
        }

        if (not opt_addr) {
            return {}; // simple_buddy out of memory
        }

        auto ins = alloc_map.insert({*opt_addr, order});
        if (not ins.second) {
            INTERNAL_TRAP(); // simple_buddy; address already in map, which indicates corrupted buddy
        }
        return opt_addr;
    }

    /**
     * Frees an address range.
     *
     * \param addr The address to free.
     */
    void free(uintptr_t addr)
    {
        this->buddy_t::free(addr, alloc_map.at(addr));
        alloc_map.erase(addr);
    }

    /**
     * Frees an address range. Is a proxy for the parent function buddy::free().
     *
     * \param addr The address to free.
     * \param order The order of the address range to free.
     */
    void free(uintptr_t addr, math::order_t order) { buddy_t::free(addr, order); }
};

using simple_buddy = simple_buddy_impl<>;
