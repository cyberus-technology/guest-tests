/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "algorithm"
#include "cbl/math.hpp"
#include "stdint.h"
#include "stdio.h"
#include <logger/trace.hpp>

#define heap_assert(x, y) ASSERT(x, y)
#define heap_max(x, y) std::max(x, y)
