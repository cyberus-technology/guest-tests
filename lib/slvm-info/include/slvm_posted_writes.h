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

#include <cbl/lock_free_queue_def.h>

#include <stdint.h>

struct posted_writes_descriptor
{
    uint64_t addr;
    uint64_t size;
    uint64_t buf;
};

#define POSTED_WRITES_PAGE_SIZE 4096

/*
 * Our queue must fit into a single 4k page and we use the maximum number of
 * elements so it fits. The user of the posted write API is able to dynamically
 * set the number of elements that will be enqueued via the vmcall API.
 */
#define POSTED_WRITES_QUEUE_SIZE                                                                                       \
    (((POSTED_WRITES_PAGE_SIZE - sizeof(lock_less_queue_t)) / sizeof(posted_writes_descriptor)) - 1)

#define POSTED_WRITES_QUALIFICATION_BITS_AVAILABLE 3

/**
 * The posted writes mechanism is able to communicate with the userspace
 * VMM by setting certain vmexit qualication bits on a TSC_TIMEOUT exit. A
 * normal TSC_TIMEOUT exit does not have a valid exit qualication so it is
 * safe to use it.
 *
 * NOTE: Currently, our posted writes design specifies 3 bits to transmit a
 * value. Consider adapting the design if we exceed the available bits.
 */
enum posted_writes_qualifications
{
    BUFFER_FULL = 1,
    UNABLE_TO_EMULATE = 2,
};
