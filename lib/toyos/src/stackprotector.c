/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

// We explicitly use no dependencies here to avoid dreaded dependency cycles.

#if __SIZEOF_POINTER__ != __SIZEOF_LONG__
#error "Please update this file for your pointer size."
#endif

// Use a non-canonical address as stack canary to get instant #GP on use.
__attribute__((used)) const unsigned long __stack_chk_guard = 0xfefefefefefefefeul;

__attribute__((used, noreturn)) void __stack_chk_fail()
{
   __builtin_trap();
}
