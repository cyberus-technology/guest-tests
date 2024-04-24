// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#if defined(__cplusplus)
#include <toyos/util/trace.hpp>
#define assert(x) ASSERT(x, "Assertion failed")
#else
#define assert(x) \
    do {          \
    } while (0)
#endif
