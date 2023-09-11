/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "optional"
#include "string"

#include <compiler.hpp>

class console
{
public:
   virtual ~console() {}

   void puts_default(const std::string& str)
   {
      for (const char& c : str) {
         putc(c);
      }
   }

   virtual void puts(const std::string& str) = 0;

   virtual void putc(char c) = 0;

   static bool is_line_ending(const char c)
   {
      return c == '\n' or c == '\r';
   }
};
