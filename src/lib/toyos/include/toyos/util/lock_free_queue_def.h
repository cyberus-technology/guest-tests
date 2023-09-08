/*
  SPDX-License-Identifier: MIT

  Copyright (c) 2022-2023 Cyberus Technology GmbH

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <stdint.h>
/**
 * This file contains the definitions for a single-producer,
 * single-consumer, lock-free queue.
 *
 * The data layout in memory looks like the following:
 *
 *
 *    low addresses       offset in bytes
 *         ^
 *         |
 * |----------------|       0
 * | meta_data      |
 * |----------------|       64
 * | read_position  |
 * |----------------|       128
 * | write_position |
 * |----------------|       192
 * | entry_0        |
 * |----------------|       192 + 1 * sizeof(entry)
 * | entry_1        |
 * |----------------|       192 + 2 * sizeof(entry)
 * | ...            |
 * |----------------|       192 + n * sizeof(entry)
 * | entry_n        |
 * |----------------|
 * | entry_n+1      |
 * |----------------|
 *         |
 *         v
 *   high addresses
 *
 * Number of Entries
 *   The implementation must reserve one additional entry slot so we can distinguish between
 *   an empty and a full buffer. See Empty/Full checking.
 *
 * Accessing Read and Write Position
 *   Each access to both read_position and write_position must happen atomically.
 *
 * Empty Buffer Checking:
 *   The implementation has to check whether read_position equals write_position.
 *   If this check evaluates to true, the buffer is empty, if not, it contains
 *   at least one element.
 *
 * Full Buffer Checking:
 *   The implementation has to check whether the read position equals the next
 *   possible write slot. If this check evaluates to true, the buffer is full.
 *   If it evaluates to false, the buffer can at least hold one additional
 *   element.
 *
 * Inserting a New Element:
 *   The implementation first has to check whether the buffer is already full
 *   and fail if this check evaluates to true. Otherwise, the implementation is
 *   supposed to first write the new entry into the slot as indicated by
 *   write_position, and increase write_position afterwards.
 *
 * Removing the Top-Of-Queue Item
 *   The implementation first has to check whether the buffer is empty and
 *   indicate a failure if this check evaluates to true. Otherwise,
 *   the read_position needs to be updated to the next possible slot.
 *
 * Accessing the Top-Of-Queue Item
 *   The implementation first needs to check whether the buffer is empty
 *   and indicate a failure in that case. Otherwise, the entry matching the
 *   read_position can be accessed.
 */

#define LFQ_CACHE_LINE_SIZE (64)

#define LFQ_API_VERSION (1)

struct lock_less_queue_meta_t
{
   uint64_t version;
   uint64_t entry_num;
   uint64_t entry_size;
};

// We align both read_position and write_position to cache line boundaries to prevent false-sharing.
struct alignas(LFQ_CACHE_LINE_SIZE) lock_less_queue_t
{
   lock_less_queue_meta_t meta_data;                      // offset 0
   alignas(LFQ_CACHE_LINE_SIZE) uint64_t read_position;   // offset 64
   alignas(LFQ_CACHE_LINE_SIZE) uint64_t write_position;  // offset 128
                                                          // The data follows here. The first data element also needs to be aligned to CACHE_LINE_SIZE.
};
