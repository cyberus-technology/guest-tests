/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "toyos/util/math.hpp"
#include <compiler.hpp>

#include <stddef.h>

EXTERN_C uintptr_t irq_handlers[256];

struct __PACKED__ idt
{
   struct __PACKED__ entry
   {
      volatile uint16_t off_lo;
      volatile uint16_t cs;
      volatile uint16_t ar;
      volatile uint16_t off_hi;
      volatile uint32_t off_64b;
      volatile uint32_t reserved;

      void configure(uintptr_t offset)
      {
         off_lo = offset;
         cs = 8;
         ar = 0x8e00;  // P=1, DPL=0, 64-bit INTR gate, IST=0
         off_hi = offset >> 16;
         off_64b = offset >> 32;
      }

      void set_ist(uint8_t ist)
      {
         constexpr size_t IST_MASK{ math::mask(3) };
         ar &= ~IST_MASK;
         ar |= (ist & IST_MASK);
      }
   };

   struct __PACKED__ descriptor
   {
      uint16_t limit;
      uintptr_t base;
   };

   entry entries[256];

   idt()
   {
      for (size_t idx{ 0 }; idx < 256; idx++) {
         entries[idx].configure(irq_handlers[idx]);
      }

      load();
   }

   void load()
   {
      descriptor desc{ uint16_t(sizeof(entries) - 1), uintptr_t(entries) };
      asm volatile("lidt %[idt]" ::[idt] "m"(desc)
                   : "memory");
   }
};

struct __PACKED__ intr_regs
{
   uint64_t r15, r14, r13, r12;
   uint64_t r11, r10, r9, r8;
   uint64_t rdi, rsi, rbp;
   uint64_t rdx, rcx, rbx, rax;

   uint64_t vector, error_code;

   uint64_t rip, cs, flags, rsp, ss;
};

EXTERN_C void irq_entry(intr_regs* regs);
