/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <hedron/cap_sel.hpp>

#include <stdint.h>

class semaphore
{
public:
    /**
     * \param initial_counter The initial value of the semaphore counter
     * \param selector        The capability selector for the semaphore object
     * \param pd              The owner (PD) of the semaphore. If this is not ~0ul, the object will be created.
     */
    explicit semaphore(uint64_t initial_counter = 0, hedron::cap_sel selector = ~0ul);

    void up();

    /**
     * \param zero If true, the counter will be forced to zero
     */
    void down(bool zero = false) { down_timeout(0, zero); }

    /**
     * \param timeout The absolute TSC target at which this operation should time out.
     * \param zero    If true, the counter will be forced to zero
     * \return True, if the operation timed out.
     */
    bool down_timeout(uint64_t timeout, bool zero = false);

    hedron::cap_sel get_sel() const { return selector_; }

private:
    hedron::cap_sel selector_;
};

class user_semaphore
{
public:
    user_semaphore(uint64_t initial_counter = 0, hedron::cap_sel selector = ~0ul)
        : sm(0, selector), counter(initial_counter)
    {}

    void up();
    void down();

    class guard
    {
    public:
        explicit guard(user_semaphore& sm_) : sm(sm_) { sm.down(); }
        ~guard() { sm.up(); }

    private:
        user_semaphore& sm;
    };

    class optional_guard
    {
    public:
        explicit optional_guard(user_semaphore* sm_) : sm(sm_)
        {
            if (sm) {
                sm->down();
            }
        }

        ~optional_guard()
        {
            if (sm) {
                sm->up();
            }
        }

    private:
        user_semaphore* sm;
    };

private:
    semaphore sm;
    int64_t counter;
};
