/*
  SPDX-License-Identifier: MIT

  Copyright (c) 2021-2023 Cyberus Technology GmbH

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

#include <cstddef>
#include <cstdint>

namespace hedron
{
struct mtd_bits
{
public:
    enum field : uint64_t
    {
        ACDB = 1u << 0,
        BSD = 1u << 1,
        ESP = 1u << 2,
        EIP = 1u << 3,
        EFL = 1u << 4,
        DSES = 1u << 5,
        FSGS = 1u << 6,
        CSSS = 1u << 7,
        TR = 1u << 8,
        LDTR = 1u << 9,
        GDTR = 1u << 10,
        IDTR = 1u << 11,
        CR = 1u << 12,
        DR = 1u << 13,
        SYS = 1u << 14,
        QUAL = 1u << 15,
        CTRL = 1u << 16,
        INJ = 1u << 17,
        STA = 1u << 18,
        TSC = 1u << 19,
        EFER_PAT = 1u << 20,
        PDPTE = 1u << 21,
        GPR_R8_R15 = 1u << 22,
        SYSCALL_SWAPGS = 1u << 23,
        TSC_TIMEOUT = 1u << 24,

        VINTR = 1u << 26,
        EOI = 1u << 27,
        TPR = 1u << 28,

        TLB = 1u << 30,
        FPU = 1u << 31,

        NONE = 0,
        DEFAULT = 0xFFFFFF,

        ESP_EIP = EIP | ESP,
    };

    explicit constexpr operator uint64_t() const { return raw; }
    constexpr bool operator==(const mtd_bits& o) const { return static_cast<uint64_t>(o) == raw; }

    void clear() { raw = 0; }
    void set(field f) { raw |= static_cast<uint64_t>(f); }

    bool contains(field f) const { return raw & static_cast<uint64_t>(f); }

private:
    uint64_t raw;
};

} // namespace hedron
