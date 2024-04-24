// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "stddef.h"

namespace baretest
{
    void success(const char* name);
    void failure(const char* name);
    void benchmark(const char* name, long int value, const char* unit);
    void skip();
    void hello(size_t test_count);
    void goodbye();
}  // namespace baretest
