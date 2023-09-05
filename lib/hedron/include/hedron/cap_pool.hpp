/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <cbl/interval.hpp>
#include <hedron/cap_sel.hpp>

/**
 * Initialize the capability pool with the given base and size.
 */
void initialize_cap_pool(uint64_t cap_base, uint64_t cap_size);

/** Allocates a capability range filled with null caps.
 * The allocated range is naturally aligned.
 * \param ord order to be allocated
 * \return capability range
 */
cbl::interval allocate_cap_range(size_t ord = 0);

/**
 * Allocate a single unused capability selector.
 *
 * \return a free capability selector
 */
hedron::cap_sel allocate_cap();

/**
 * Reclaims a capability range that was allocated before
 */
void reclaim_cap_range([[maybe_unused]] cbl::interval ival);
