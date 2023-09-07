/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <compiler.hpp>

struct __PACKED__ xhci_capability
{
   enum
   {
      CAPID_BITS = 8,
      CAPID_SHIFT = 0,
      NEXTP_BITS = 8,
      NEXTP_SHIFT = 8,
      CSPEC_BITS = 16,
      CSPEC_SHIFT = 16,
   };

   uint8_t id() const
   {
      return (raw >> CAPID_SHIFT) & math::mask(CAPID_BITS);
   }
   uint8_t next() const
   {
      return (raw >> NEXTP_SHIFT) & math::mask(NEXTP_BITS);
   }
   uint16_t specific() const
   {
      return (raw >> CSPEC_SHIFT) & math::mask(CSPEC_BITS);
   }

   void specific(uint16_t new_val)
   {
      raw = (raw & ~math::mask(CSPEC_BITS, CSPEC_SHIFT)) | (static_cast<uint32_t>(new_val) << CSPEC_SHIFT);
   }

   volatile uint32_t raw;
};
