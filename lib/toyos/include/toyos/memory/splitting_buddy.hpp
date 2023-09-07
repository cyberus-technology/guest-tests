/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <arch.h>
#include <toyos/util/interval.hpp>
#include <toyos/util/order_range.hpp>
#include "toyos/util/math.hpp"
#include "buddy.hpp"

#include <optional>

/**
 * \brief A specialized buddy which is able to allocate and free arbitrary sizes.
 *
 * Internally a standard buddy is used for allocation. Allocation requests are
 * aligned up to the next power of two and the overhead is returned to the
 * buddy again in power of two sized chunks.
 */
class splitting_buddy
{
public:
    explicit splitting_buddy(math::order_t max_ord) : internal_allocator(max_ord) {}

    /**
     * Allocates a range at least the requested size. Allocates a range of the
     * next power of two of size. The overhead is freed and returned to the
     * buddy.
     *
     * \size The size to allocate.
     * \return The allocated interval or an empty optional if unsuccessful.
     */
    std::optional<cbl::interval> alloc(size_t size)
    {
        const size_t alloc_order {math::order_envelope(size)};

        const auto alloc_start {internal_allocator.alloc(alloc_order)};
        if (not alloc_start) {
            return {};
        }

        const auto alloc_ival {cbl::interval::from_order(*alloc_start, alloc_order)};

        const auto num_bytes {size};
        const size_t remaining {alloc_ival.size() - num_bytes};
        const auto remaining_ival {cbl::interval::from_size(alloc_ival.a + num_bytes, remaining)};

        free(remaining_ival);

        return cbl::interval(*alloc_start, remaining_ival.a);
    }

    /**
     * Frees a given interval and makes it available for allocation again.
     *
     * \param ival The interval to free.
     */
    void free(cbl::interval ival)
    {
        for (const auto& range : cbl::order_range(ival.a, ival.size(), internal_allocator.max_order)) {
            internal_allocator.free(range.a, math::order_envelope(range.size()));
        }
    }

private:
    buddy internal_allocator;
};
