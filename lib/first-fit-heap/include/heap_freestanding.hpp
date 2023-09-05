/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <algorithm>
#include <math.hpp>
#include <stdint.h>
#include <stdio.h>
#include <trace.hpp>

#define heap_assert(x, y) ASSERT(x, y)
#define heap_max(x, y) std::max(x, y)
