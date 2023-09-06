/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <config.h>
#include <math/math.hpp>
#include <stdio.h>
#include <logger/trace.hpp>
#include <type_traits>

static_assert((STATIC_TLS_SIZE % sizeof(uint64_t)) == 0, "TLS size must be a multiple of 8");
static_assert((STATIC_TLS_SIZE & math::mask(4)) == 0x8, "Stack implementation requires bits [3:0] to be 0b1000");

namespace hedron
{
class utcb;
};

/**
 * Base layout for all stacks.
 * When changing this layout, the initial stack layout setup in init.S::_startup
 * must be changed too.
 */
struct layout_base
{
    uint64_t cpu_;
    hedron::utcb* utcb_;
};

/** This is a generic stack helper that can
 * be used to access thread local storage (TLS).
 */
template <typename LAYOUT>
class stack_base
{
private:
    static_assert(sizeof(LAYOUT) <= STATIC_TLS_SIZE);
    static_assert(std::is_base_of_v<layout_base, LAYOUT>, "Stack layout must inherit from layout_base");

    static size_t get_current_sp()
    {
        size_t sp;
        asm("mov %%rsp, %[sp];" : [sp] "=rm"(sp));
        return sp;
    }

protected:
    static LAYOUT& get_current_stack_items()
    {
        size_t sp {current_bottom()};
        ASSERT(sp != 0, "stack is null");
        return *num_to_ptr<LAYOUT>(sp);
    }

public:
    static size_t current_bottom()
    {
        size_t sp {get_current_sp()};
        return (sp & ~(STACK_SIZE_VIRT - 1)) + STACK_SIZE_VIRT - STATIC_TLS_SIZE;
    }

    static hedron::utcb& current_utcb() { return *get_current_stack_items().utcb_; }

    static uint64_t current_cpu() { return get_current_stack_items().cpu_; }
};
