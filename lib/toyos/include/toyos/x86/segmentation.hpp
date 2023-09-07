/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <assert.h>

#include "toyos/util/math.hpp"
#include <arch.h>
#include <compiler.hpp>
#include <toyos/util/cast_helpers.hpp>

namespace x86
{

   enum class segment_register
   {
      CS,
      DS,
      ES,
      FS,
      GS,
      SS,
   };

   struct gdt_segment
   {
      uint16_t sel{ 0 };
      uint16_t ar{ 0 };
      uint32_t limit{ 0 };
      uint64_t base{ 0 };

      // The protection fields according to the Intel specification
      // chapter 5.2 Fields and Flags used for segment-level and page-level protection
      // Please be aware that the offset shifts are counted from the first access
      // right bit (Accessed).
      // The weird encapsulation of limit is ignored.
      enum class ar_flag_mask : uint32_t
      {
         IA32_CODE_LONG = 1u << 9,
         DATA_BIG = 1u << 10,
         CODE_DEFAULT = 1u << 10,
         ALL_GRANULARITY = 1u << 11
      };

      bool ar_set(const ar_flag_mask flag) const
      {
         return (ar & to_underlying(flag)) > 0;
      }
   };

   struct __PACKED__ descriptor_ptr
   {
      uint16_t limit;
      uint64_t base;
      bool operator!=(const descriptor_ptr& rhs) const
      {
         return not(base == rhs.base and limit == rhs.limit);
      }
   };

   enum
   {
      SELECTOR_TI_SHIFT = 2,
      SELECTOR_INDEX_SHIFT = 3,
      SELECTOR_PL_MASK = 3,
   };

   enum class selector_ti_types : uint8_t
   {
      GDT = 0,
      LDT = 1,
   };

   struct __PACKED__ tss
   {
      uint32_t reserved;
      uint64_t rsp0, rsp1, rsp2;
      uint64_t reserved2;
      uint64_t ist1, ist2, ist3, ist4, ist5, ist6, ist7;
      uint32_t reserved3[3] = { 0 };
   };

   struct segment_selector
   {
      enum
      {
         RPL_SHIFT = 0,
         RPL_WIDTH = 2,
         TI_SHIFT = 2,
         TI_WIDTH = 1,
         INDEX_SHIFT = 3,
         INDEX_WIDTH = 13,
      };

      explicit segment_selector(uint16_t raw_)
         : raw(raw_) {}

      uint16_t index() const
      {
         return (raw >> INDEX_SHIFT) & math::mask(INDEX_WIDTH);
      };

      uint16_t value() const
      {
         return raw;
      }

      uint16_t raw;
   };

   struct gdt_entry
   {
      enum
      {
         LIMIT_LO_SHIFT = 0,
         LIMIT_LO_WIDTH = 16,
         LIMIT_HI_SHIFT = 48,
         LIMIT_HI_WIDTH = 4,

         BASE_LO_SHIFT = 16,
         BASE_LO_WIDTH = 16,
         BASE_MID_SHIFT = 32,
         BASE_MID_WIDTH = 8,
         BASE_HI_SHIFT = 56,
         BASE_HI_WIDTH = 8,
         BASE_64_SHIFT = 0,
         BASE_64_WIDTH = 32,
         BASE_LO_MASK = math::mask(BASE_LO_WIDTH, BASE_LO_SHIFT),
         BASE_MID_MASK = math::mask(BASE_MID_WIDTH, BASE_MID_SHIFT),
         BASE_HI_MASK = math::mask(BASE_HI_WIDTH, BASE_HI_SHIFT),

         AR_LO_SHIFT = 40,
         AR_LO_WIDTH = 8,
         AR_HI_SHIFT = 52,
         AR_HI_WIDTH = 4,

         AR_A_SHIFT = 0,
         AR_RW_SHIFT = 1,
         AR_CE_SHIFT = 2,
         AR_X_SHIFT = 3,
         AR_TYPE_SHIFT = 0,
         AR_TYPE_WIDTH = 4,
         AR_S_SHIFT = 4,
         AR_DPL_SHIFT = 5,
         AR_DPL_WIDTH = 2,
         AR_P_SHIFT = 7,
         AR_AVL_SHIFT = 8,
         AR_L_SHIFT = 9,
         AR_DB_SHIFT = 10,
         AR_G_SHIFT = 11,

         AR_A = 1u << AR_A_SHIFT,
         AR_RW = 1u << AR_RW_SHIFT,
         AR_CE = 1u << AR_CE_SHIFT,
         AR_X = 1u << AR_X_SHIFT,
         AR_S = 1u << AR_S_SHIFT,
         AR_DPL = 3u << AR_DPL_SHIFT,
         AR_P = 1u << AR_P_SHIFT,
         AR_AVL = 1u << AR_AVL_SHIFT,
         AR_L = 1u << AR_L_SHIFT,
         AR_DB = 1u << AR_DB_SHIFT,
         AR_G = 1u << AR_G_SHIFT,
      };

      enum class segment_type : uint8_t
      {
         RESERVED0 = 0,
         TSS_16BIT_AVAIL = 1,
         LDT = 2,
         TSS_16BIT_BUSY = 3,
         DATA_RW_ACCESS = 3,
         CALL_GATE_16BIT = 4,
         TASK_GATE = 5,
         INT_GATE_16BIT = 6,
         TRAP_GATE_16BIT = 7,
         RESERVED1 = 8,
         TSS_32BIT_AVAIL = 9,
         TSS_64BIT_AVAIL = 9,
         RESERVED2 = 10,
         TSS_BUSY_32BIT = 11,
         TSS_BUSY_64BIT = 11,
         CODE_RX_ACCESS = 11,
         CALL_GATE_32BIT = 12,
         CALL_GATE_64BIT = 12,
         RESERVED3 = 13,
         INT_GATE_32BIT = 14,
         INT_GATE_64BIT = 14,
         TRAP_GATE_32BIT = 15,
         TRAP_GATE_64BIT = 15,
      };

      uint16_t ar() const
      {
         return ((raw >> AR_LO_SHIFT) & math::mask(AR_LO_WIDTH))
                | (((raw >> AR_HI_SHIFT) & math::mask(AR_HI_WIDTH)) << AR_LO_WIDTH);
      }

      bool g() const
      {
         return ar() & AR_G;
      }
      bool system() const
      {
         return not(ar() & AR_S);
      }  // System segments have S=0
      bool present() const
      {
         return ar() & AR_P;
      }
      bool is_code() const
      {
         return (not system()) and (ar() & AR_X);
      }
      bool is_conforming() const
      {
         return is_code() and (ar() & AR_CE);
      }
      uint8_t dpl() const
      {
         return (ar() & AR_DPL) >> AR_DPL_SHIFT;
      }
      segment_type type() const
      {
         return static_cast<segment_type>(ar() & math::mask(AR_TYPE_WIDTH));
      }

      uint32_t limit() const
      {
         uint32_t lim = ((raw >> LIMIT_LO_SHIFT) & math::mask(LIMIT_LO_WIDTH))
                        | ((raw >> LIMIT_HI_SHIFT) & math::mask(LIMIT_HI_WIDTH)) << LIMIT_LO_WIDTH;
         if(g() and not system()) {
            lim = (lim << PAGE_BITS) + PAGE_SIZE - 1;
         }
         return lim;
      }

      uint64_t base() const
      {
         uint64_t result{ ((raw >> BASE_LO_SHIFT) & math::mask(BASE_LO_WIDTH))
                          | ((raw >> BASE_MID_SHIFT) & math::mask(BASE_MID_WIDTH)) << BASE_LO_WIDTH
                          | ((raw >> BASE_HI_SHIFT) & math::mask(BASE_HI_WIDTH)) << (BASE_MID_WIDTH + BASE_LO_WIDTH) };

         if(g() and system()) {
            result |= (raw_high & math::mask(BASE_64_WIDTH)) << (BASE_HI_WIDTH + BASE_MID_WIDTH + BASE_LO_WIDTH);
         }

         return result;
      }

      uint32_t base32() const
      {
         return ((raw >> BASE_LO_SHIFT) & math::mask(BASE_LO_WIDTH))
                | ((raw >> BASE_MID_SHIFT) & math::mask(BASE_MID_WIDTH)) << BASE_LO_WIDTH
                | ((raw >> BASE_HI_SHIFT) & math::mask(BASE_HI_WIDTH)) << (BASE_MID_WIDTH + BASE_LO_WIDTH);
      }

      void set_conforming(bool c) volatile
      {
         raw &= ~(1ull << (AR_LO_SHIFT + AR_CE_SHIFT));
         raw |= c * (1ull << (AR_LO_SHIFT + AR_CE_SHIFT));
      }

      void set_type(segment_type t)
      {
         set_ar((ar() & ~math::mask(AR_TYPE_WIDTH)) | to_underlying(t));
      }

      uint16_t set_bit(uint16_t value, bool bit_state, uint16_t bit_nr) const
      {
         return (value & ~(1 << bit_nr)) | (bit_state << bit_nr);
      }

      void set_ar(uint16_t value)
      {
         raw &= ~math::mask(AR_LO_WIDTH, AR_LO_SHIFT);
         raw &= ~math::mask(AR_HI_WIDTH, AR_HI_SHIFT);
         raw |= (value & math::mask(AR_LO_WIDTH)) << AR_LO_SHIFT;
         raw |= ((value >> AR_LO_WIDTH) & math::mask(AR_HI_WIDTH)) << AR_HI_SHIFT;
      }

      void set_g(bool g)
      {
         set_ar(set_bit(ar(), g, AR_G_SHIFT));
      }
      void set_system(bool s)
      {
         set_ar(set_bit(ar(), not s, AR_S_SHIFT));
      }  // System segments have S=0
      void set_present(bool p)
      {
         set_ar(set_bit(ar(), p, AR_P_SHIFT));
      }
      void set_db(bool db)
      {
         set_ar(set_bit(ar(), db, AR_DB_SHIFT));
      }

      void set_limit(uint32_t l)
      {
         raw &= ~(math::mask(LIMIT_LO_WIDTH, LIMIT_LO_SHIFT) | math::mask(LIMIT_HI_WIDTH, LIMIT_HI_SHIFT));
         raw |= math::mask(LIMIT_LO_WIDTH) & l;
         raw |= ((l >> LIMIT_LO_WIDTH) & math::mask(LIMIT_HI_WIDTH)) << LIMIT_HI_SHIFT;
      }

      void set_base(uintptr_t addr)
      {
         raw &= ~(BASE_LO_MASK | BASE_MID_MASK | BASE_HI_MASK);
         raw |= (((addr & math::mask(BASE_LO_WIDTH)) << BASE_LO_SHIFT)
                 | (((addr >> BASE_LO_WIDTH) & math::mask(BASE_MID_WIDTH)) << BASE_MID_SHIFT)
                 | (((addr >> (BASE_LO_WIDTH + BASE_MID_WIDTH)) & math::mask(BASE_HI_WIDTH)) << BASE_HI_SHIFT));

         if(g() and system()) {
            raw_high &= ~math::mask(BASE_64_WIDTH, BASE_64_SHIFT);
            raw_high |= ((addr >> BASE_64_WIDTH) & math::mask(BASE_64_WIDTH)) << BASE_64_SHIFT;
         }
      }

      uint64_t raw{ 0 };
      uint64_t raw_high{ 0 };
   };

   gdt_entry* get_gdt_entry(descriptor_ptr gdtr, segment_selector s);

}  // namespace x86
