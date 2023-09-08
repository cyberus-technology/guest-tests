/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "array"

#include "toyos/util/math.hpp"
#include <compiler.hpp>

#include "usb_strings.hpp"

struct __PACKED__ dbc_info_context
{
   dbc_info_context()
   {
      data.fill(0);
   }
   union
   {
      struct
      {
         volatile uint64_t string0;
         volatile uint64_t manufacturer;
         volatile uint64_t product;
         volatile uint64_t serial;

         volatile uint8_t string0_length, manufacturer_length, product_length, serial_length;
      };
      std::array<uint8_t, CONTEXT_SIZE> data;
   };
};
static_assert(sizeof(dbc_info_context) == CONTEXT_SIZE, "Debug contexts need to be 64 bytes large!");

struct __PACKED__ string_descriptors
{
   usb_string_t string0, manufacturer, product, serial;
};

struct __PACKED__ dbc_endpoint_context
{
   enum
   {
      BULK_OUT = 2,
      BULK_IN = 6,

      PRODUCER_CYCLE_STATE = 1,

      TYPE_BITS = 3,
      TYPE_SHIFT = 3,
   };

   dbc_endpoint_context()
   {
      data.fill(0);
   }

   uint8_t type() const
   {
      return (settings >> TYPE_SHIFT) & math::mask(TYPE_BITS);
   }
   void type(uint8_t t)
   {
      settings &= ~math::mask(TYPE_BITS, TYPE_SHIFT);
      settings |= ((t & math::mask(TYPE_BITS)) << TYPE_SHIFT);
   }

   void set_dequeue_ptr(uintptr_t ptr)
   {
      dequeue_ptr = ptr | PRODUCER_CYCLE_STATE;
   }

   union
   {
      struct
      {
         volatile uint32_t status_info;
         volatile uint8_t settings;
         volatile uint8_t max_burst_size;
         volatile uint16_t max_packet_size;

         volatile uint64_t dequeue_ptr;

         volatile uint16_t average_trb;
         volatile uint16_t max_esit_payload;
      };
      std::array<uint8_t, CONTEXT_SIZE> data;
   };
};
static_assert(sizeof(dbc_endpoint_context) == CONTEXT_SIZE, "Debug contexts need to be 64 bytes large!");
