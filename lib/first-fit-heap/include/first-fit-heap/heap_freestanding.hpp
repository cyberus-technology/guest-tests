/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "algorithm"
#include "toyos/util/math.hpp"
#include "stdint.h"
#include "stdio.h"

#define heap_assert(x, y) ASSERT(x, y)
#define heap_max(x, y) std::max(x, y)
