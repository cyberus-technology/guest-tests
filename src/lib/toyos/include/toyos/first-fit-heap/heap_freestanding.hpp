// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "algorithm"
#include "stdint.h"
#include "stdio.h"
#include "toyos/util/math.hpp"

#define heap_assert(x, y) ASSERT(x, y)
#define heap_max(x, y) std::max(x, y)
