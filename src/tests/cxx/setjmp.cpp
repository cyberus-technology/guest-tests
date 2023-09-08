/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <setjmp.h>

#include <toyos/baretest/baretest.hpp>

TEST_CASE(setjmp_should_return_null_on_direct_call)
{
   jmp_buf env;
   int ret{ setjmp(env) };

   BARETEST_ASSERT(ret == 0);
}

void test_setjmp_generic(int val, int expected)
{
   jmp_buf env;
   int ret{ setjmp(env) };

   if(ret == 0) {
      longjmp(env, val);
   }

   BARETEST_ASSERT(ret == expected);
}

TEST_CASE(longjmp_should_unwind_with_positive_return_value)
{
   test_setjmp_generic(1, 1);
}

TEST_CASE(longjmp_should_unwind_with_negative_return_value)
{
   test_setjmp_generic(-1337, -1337);
}

TEST_CASE(longjmp_with_0_should_return_1)
{
   test_setjmp_generic(0, 1);
}
