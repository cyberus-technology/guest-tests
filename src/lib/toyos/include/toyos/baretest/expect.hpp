/*
 * Copyright 2017 Cyberus Technology GmbH

 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "assert.hpp"
#include "print.hpp"

namespace baretest
{

   template<typename T>
   class expectation
   {
      const T expected_value;

      void print_result(bool result, const T& other) const
      {
         if (not result) {
            printf("Expect: Mismatch: expected: ");
            print(expected_value);
            printf("Actual: ");
            print(other);
            printf(" \n");
         }
      }

   public:
      expectation(const T& expect)
         : expected_value(expect) {}

      bool operator==(const T& other)
      {
         bool result = (expected_value == other);
         print_result(result, other);
         return result;
      }

      bool operator!=(const T& other)
      {
         bool result = not(expected_value == other);
         print_result(result, other);
         return result;
      }

      bool operator>(const T& other)
      {
         bool result = (expected_value > other);
         print_result(result, other);
         return result;
      }

      bool operator>=(const T& other)
      {
         bool result = (expected_value >= other);
         print_result(result, other);
         return result;
      }

      bool operator<(const T& other)
      {
         bool result = (expected_value < other);
         print_result(result, other);
         return result;
      }

      bool operator<=(const T& other)
      {
         bool result = (expected_value <= other);
         print_result(result, other);
         return result;
      }
   };

}  // namespace baretest

#define BARETEST_VERIFY(stmt)                                                      \
   do {                                                                            \
      if (not(stmt)) {                                                             \
         baretest::fail("Expect: %s failed @ %s:%d\n", #stmt, __FILE__, __LINE__); \
      }                                                                            \
   } while (false);
