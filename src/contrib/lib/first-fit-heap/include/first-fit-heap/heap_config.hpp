/*
 * MIT License

 * Copyright (c) 2016 Thomas Prescher

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

// For a free-standing heap, the following functions/structures have to be defined
//     * placement new
//     * size_t and uint*_t types
//     * heap_max(x, y)
//     * heap_assert(cond, str) if ENABLE_HEAP_ASSERT is set

#if !(HEAP_LINUX || HEAP_FREESTANDING)
#error "invalid heap config, you need to define either HEAP_LINUX or HEAP_FREESTANDING"
#endif

#ifdef HEAP_LINUX
#include <algorithm>
#include <assert.h>
#include <new>
#include <stdint.h>
#include <sys/types.h>
#define HEAP_MAX(x, y) std::max(x, y);
#else
#include "heap_freestanding.hpp"
#define HEAP_MAX(x, y) heap_max(x, y);
#endif

#ifdef HEAP_ENABLE_ASSERT
#ifdef HEAP_LINUX
#define ASSERT_HEAP(cond) assert(cond)
#else
#define ASSERT_HEAP(cond) heap_assert(cond, #cond)
#endif
#else
#define ASSERT_HEAP(cond) ;
#endif

#define HEAP_PACKED __attribute__((packed))
#define HEAP_UNUSED __attribute__((unused))
