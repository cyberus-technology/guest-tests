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

/* This file contains the Hedron UTCB layout structs without any dependencies on supernova internal libraries or other
 * headers (except standard C / C++ headers). The intention is to ship this as part of other software that need to be
 * aware of Hedron's UTCB layout. */

#pragma once

#include <cstddef>
#include <cstdint>

#include "mtd.hpp"

namespace hedron
{

/**
 * Hedron Compressed Segment Descriptor
 *
 * This is Hedron's way of storing segment descriptors. Please beware, it is slightly different from the hardware
 * layout.
 */
struct segment_descriptor
{
    enum
    {
        SEGMENT_AR_L = 1u << 9,
        SEGMENT_AR_DB = 1u << 10,
        SEGMENT_AR_G = 1u << 11,
    };

    uint16_t sel;
    uint16_t ar;
    uint32_t limit;
    uint64_t base;

    bool l() const { return ar & SEGMENT_AR_L; }
    bool db() const { return ar & SEGMENT_AR_DB; }
    bool g() const { return ar & SEGMENT_AR_G; }
};

/**
 * VM Interruptibility State
 *
 * This struct is a convenience wrapper for the VM Interruptibility State. Refer to Intel SDM Vol 3. Chapter "Virtual
 * Machine Control Structures" Section "Guest Non-Register State".
 */
class intr_state
{
    uint32_t raw;

public:
    enum
    {
        STI_BLOCKING = 1 << 0,
        MOV_SS_BLOCKING = 1 << 1,
        SMI_BLOCKING = 1 << 2,
        NMI_BLOCKING = 1 << 3,
    };

    bool is_sti_blocking() const { return raw & STI_BLOCKING; }
    bool is_mov_ss_blocking() const { return raw & MOV_SS_BLOCKING; }
    bool is_smi_blocking() const { return raw & SMI_BLOCKING; }
    bool is_nmi_blocking() const { return raw & NMI_BLOCKING; }

    void set_sti_blocking() { raw |= STI_BLOCKING; }
    void clear_sti_blocking() { raw &= ~STI_BLOCKING; }

    void set_mov_ss_blocking() { raw |= MOV_SS_BLOCKING; }
    void clear_mov_ss_blocking() { raw &= ~MOV_SS_BLOCKING; }

    void set(uint32_t mask) { raw |= mask; }
    void clear(uint32_t mask) { raw &= ~mask; }

    explicit operator uint32_t() const { return raw; }
};

/**
 * VM Exit Interrupt Information
 *
 * This struct is a convenience wrapper around interrupt information, as it is contained in the VMCS VM-exit interrupt
 * information field. Refer to Intel SDM Vol 3. Chapter "Virtual Machine Control Structures" Section "Information for VM
 * Exits Due to Vectored Events".
 */
struct intr_info
{
    enum
    {
        TYPE_SHIFT = 8,
        ERROR_SHIFT = 11,
        INTWIN_SHIFT = 12,
        NMIWIN_SHIFT = 13,
        VALID_SHIFT = 31,

        VECTOR_BITS = 8,
        TYPE_BITS = 3,

        VECTOR_MASK = (1U << VECTOR_BITS) - 1,
        TYPE_MASK = ((1U << TYPE_BITS) - 1) << TYPE_SHIFT,
        ERROR_MASK = 1U << ERROR_SHIFT,
        INTWIN_MASK = 1U << INTWIN_SHIFT,
        NMIWIN_MASK = 1U << NMIWIN_SHIFT,
        VALID_MASK = 1U << VALID_SHIFT,
    };

    void clear() { raw &= NMIWIN_MASK | INTWIN_MASK; }

    void set_bits(uint32_t clr_mask, uint32_t set_mask) { raw = (raw & ~clr_mask) | set_mask; }

    uint8_t vector() const { return raw & VECTOR_MASK; }
    void vector(uint8_t vec) { set_bits(VECTOR_MASK, vec); }

    uint8_t type() const { return (raw & TYPE_MASK) >> TYPE_SHIFT; }
    void type(uint8_t t) { set_bits(TYPE_MASK, t << TYPE_SHIFT); }

    bool error() const { return raw & ERROR_MASK; }
    void error(bool e) { set_bits(ERROR_MASK, e * ERROR_MASK); }

    bool interrupt_window_requested() const { return raw & INTWIN_MASK; }
    void request_interrupt_window() { set_bits(INTWIN_MASK, INTWIN_MASK); }

    bool nmi_window_requested() const { return raw & NMIWIN_MASK; }
    void request_nmi_window() { set_bits(NMIWIN_MASK, NMIWIN_MASK); }

    bool valid() const { return raw & VALID_MASK; }
    void valid(bool v) { set_bits(VALID_MASK, v * VALID_MASK); }

    uint32_t raw;
    explicit operator uint32_t() const { return raw; }
};

enum vm_exit_flags
{
    // An NMI is pending. This flag is only relevant for passthrough VMs.
    NMI_PENDING = 1U << 0,
};

/**
 * Layout of a Hedron userspace thread control block.
 *
 * This struct contains the state of threads and vCPUs after exceptions or VM exits. It also serves as message passing
 * buffer. For details, refer to the Hedron documentation.
 */
struct utcb_layout
{
    struct utcb_header
    {
        uint16_t num_untyped;
        uint16_t num_typed;
        uint32_t padding;
        uint64_t crd_win_t;
        uint64_t crd_win_d;
        uint64_t tls;
    } header;

    static constexpr size_t UTCB_SIZE {4096};
    static constexpr size_t NUM_ITEMS {UTCB_SIZE / 8 - sizeof(utcb_header) / 8};

    union
    {
        struct
        {
            mtd_bits mtd;
            uint64_t ins_len;

            uint64_t ip;
            uint64_t rflags;

            intr_state int_state;
            uint32_t act_state;
            intr_info inj_info;
            uint32_t inj_error;
            intr_info vect_info;
            uint32_t vect_error;

            uint64_t ax;
            uint64_t cx;

            uint64_t dx;
            uint64_t bx;

            uint64_t sp;
            uint64_t bp;

            uint64_t si;
            uint64_t di;

            uint64_t r8;
            uint64_t r9;

            uint64_t r10;
            uint64_t r11;

            uint64_t r12;
            uint64_t r13;

            uint64_t r14;
            uint64_t r15;

            uint64_t qual[2];

            uint32_t ctrl[2];
            uint64_t xcr0;

            uint64_t cr0, cr2, cr3, cr4;
            uint64_t pdpte[4];

            uint64_t cr8;
            uint64_t efer;
            uint64_t pat;

            uint64_t star, lstar, fmask, kernel_gs_base;

            uint64_t dr7;
            uint64_t sysenter_cs;

            uint64_t sysenter_sp;
            uint64_t sysenter_ip;

            segment_descriptor es;
            segment_descriptor cs;
            segment_descriptor ss;
            segment_descriptor ds;
            segment_descriptor fs;
            segment_descriptor gs;

            segment_descriptor ldtr;
            segment_descriptor tr;
            segment_descriptor gdtr;
            segment_descriptor idtr;

            uint64_t tsc_val;
            uint64_t tsc_off;

            uint32_t tsc_aux, exc_bitmap;
            uint32_t tpr_threshold;
            uint32_t reserved;

            uint64_t eoi_exit_bitmap[4];

            uint16_t vintr_status;
            uint16_t reserved_array[3];

            uint64_t cr0_mon, cr4_mon;

            uint64_t spec_ctrl;
            uint64_t tsc_timeout;

            uint32_t vcpu_exit_reason;
            uint32_t vcpu_exit_flags;
        };

        struct
        {
            uint64_t items[NUM_ITEMS];
        };
    };
};

static_assert(sizeof(segment_descriptor) == 16, "ABI violation in struct segment_descriptor");
static_assert(sizeof(intr_state) == 4, "ABI violation in struct intr_state");
static_assert(sizeof(intr_info) == 4, "ABI violation in struct intr_info");

} // namespace hedron
