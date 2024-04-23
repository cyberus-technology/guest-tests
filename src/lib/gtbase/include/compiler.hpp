// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <compiler.h>

struct uncopyable
{
    uncopyable() = default;
    uncopyable(const uncopyable&) = delete;
    uncopyable& operator=(const uncopyable&) = delete;
};

struct unmovable
{
    unmovable() = default;
    unmovable(unmovable&&) = delete;
    unmovable& operator=(unmovable&&) = delete;
};
