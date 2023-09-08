/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "idt.hpp"
#include <functional>

extern idt global_idt;

using irq_handler_t = std::function<void(intr_regs*)>;

namespace irq_handler
{

   void set(const irq_handler_t& new_handler);

   class guard
   {
   public:
      guard(const irq_handler_t& new_handler);
      ~guard();

   private:
      irq_handler_t old_handler;
   };

}  // namespace irq_handler
