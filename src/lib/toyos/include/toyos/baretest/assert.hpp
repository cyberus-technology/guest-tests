// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "csetjmp"
#include "cstdarg"

namespace baretest
{

    constexpr int ASSERT_FAILED = 1;

    __attribute__((noreturn)) void fail(const char* msg, ...);

}  // namespace baretest

#define BARETEST_ASSERT(cond)                              \
    do {                                                   \
        if (not(cond)) {                                   \
            baretest::fail(                                \
                "Assertion failed @ %s:%d: '" #cond "'\n", \
                __FILE__,                                  \
                __LINE__);                                 \
        }                                                  \
    } while (0);
