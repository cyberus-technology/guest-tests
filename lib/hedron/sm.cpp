/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <cbl/interval.hpp>
#include <hedron/cap_range.hpp>
#include <hedron/sm.hpp>
#include <hedron/syscalls.hpp>
#include <libsn.hpp>
#include <trace.hpp>

semaphore::semaphore(uint64_t initial_counter, hedron::cap_sel selector) : selector_(selector)
{
    if (selector == ~0ul) {
        auto cap_ival = allocate_cap_range();
        selector_ = cap_ival.a;
        create_semaphore(selector_, initial_counter);
    }
}

void semaphore::up()
{
    auto res = hedron::sm_ctrl(selector_, false);
    ASSERT(res == hedron::SUCCESS, "unable to control SM (status: %u, cap: %#x)", res, selector_);
}

bool semaphore::down_timeout(uint64_t timeout, bool zero)
{
    auto res = hedron::sm_ctrl(selector_, true, zero, timeout);
    ASSERT(res == hedron::SUCCESS or res == hedron::COM_TIM, "unable to control SM (status: %u, cap: %#x)", res,
           selector_);
    return res == hedron::COM_TIM;
}

void user_semaphore::up()
{
    if (__sync_fetch_and_add(&counter, 1) < 0) {
        sm.up();
    }
}

void user_semaphore::down()
{
    if (__sync_fetch_and_sub(&counter, 1) <= 0) {
        sm.down();
    }
}
