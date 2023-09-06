/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "stdint.h"

#include "atomic"

#include "cbl/cast_helpers.hpp"
#include "heap.hpp"
#include "cbl/math.hpp"

/**
 * A simple heap that can only allocate memory.
 * Reclamation is not supported (nor intended).
 */
template <size_t ALIGNMENT = HEAP_MIN_ALIGNMENT>
class simple_heap
{
public:
    simple_heap(const memory& mem_) : mem(mem_) { current_head.store(mem.base()); }

    void* alloc(size_t size)
    {
        size = sanitize_size(size);
        auto addr = current_head.fetch_add(size);

        if ((addr + size) >= mem.end()) {
            return nullptr;
        }

        return num_to_ptr<void>(addr);
    }

    // This free is a NOP.
    void free(void*) {}

private:
    size_t sanitize_size(size_t s) { return math::align_up(s, math::order_max(ALIGNMENT)); }

    const memory& mem;
    std::atomic<uintptr_t> current_head;
};
