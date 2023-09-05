/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <chrono>

void initialize_delay_subsystem(uint32_t tsc_freq_khz);

void udelay(uint64_t us);

template <typename Rep, typename Period>
void delay(std::chrono::duration<Rep, Period> duration)
{
    udelay(std::chrono::duration_cast<std::chrono::microseconds>(duration).count());
}
